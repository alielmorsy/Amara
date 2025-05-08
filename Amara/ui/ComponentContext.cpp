#include "ComponentContext.h"

#include <unordered_set>

#include "../runtime/WidgetHolder.h"
#include "../runtime/IEngine.h"
#include "Widget.h"
#include "../utils/ScopedTimer.h"

// Currently I am using a placeholder useState. The real implementation should be a queue to handle setStates in order.
std::tuple<StateWrapper *, SetStateFunction> ComponentContext::useState(std::unique_ptr<StateWrapper> value,
                                                                        EmptyFunction notifier) {
    StateWrapper *wrapper;
    size_t currentIndex;
    if (_reconciliationStarted) {
        State &state = states[hookCount];
        wrapper = state.object.get();
        currentIndex = hookCount++;
    } else {
        State state{std::move(value)};
        currentIndex = states.capacity();
        wrapper = state.object.get();
        states.push_back(std::move(state));
    }


    SetStateFunction setState = [this, currentIndex, notifier=std::move(notifier)
            ](std::unique_ptr<StateWrapper> newValue) {
        auto &state = states[currentIndex];
        // Using the pointer directly as we cannot pass the unique pointer inside.
        std::unique_ptr<StateWrapper> valueToUse;
        if (newValue->getValueRef().isObject()) {
            auto obj = newValue->getValueRef().asObject(newValue->rt);
            if (obj.isFunction(newValue->rt)) {
                valueToUse = newValue->call(state.object->getInternalValue()->getValue());
            } else {
                valueToUse = std::move(newValue);
            }
        } else {
            valueToUse = std::move(newValue);
        }
        if (state.object->equals(valueToUse.get())) {
            return;
        }

        //Notify parent if needed
        notifier();
        toBeUpdated[currentIndex] = std::move(valueToUse);
        dirty = true;
    };

    return {wrapper, std::move(setState)};
}

void ComponentContext::effect(StateWrapperRef fn, std::vector<StateWrapperRef> deps) {
    if (_reconciliationStarted) {
        fn->call();
        return;
    }
    //The initial register call
    auto result = fn->call();

    effects.emplace_back(std::move(fn), std::move(deps), std::move(result));
}

void ComponentContext::_updateStates() {
    std::vector<size_t> keysToErase;
    for (auto &data: toBeUpdated) {
        states[data.first].object->setValue(data.second);
        updatedStates[data.first] = std::move(data.second);
        keysToErase.push_back(data.first);
    }
    for (const auto &key: keysToErase) {
        toBeUpdated.erase(key);
    }
}

void ComponentContext::update() {
    if (!dirty) {
        updatedStates.clear();
        hookCount = 0;
        return;
    }


    _updateStates();
    for (auto &effect: effects) {
        if (effect.cleanup->hasValue()) {
            effect.cleanup->call();
        }
        auto &deps = effect.deps;
        if (deps.empty()) continue;
        auto needRecall = false;
        for (auto &dep: deps) {
            for (auto &pair: updatedStates) {
                if (states[pair.first].object->equals(dep.get())) {
                    needRecall = true;
                }
            }
        }
        if (needRecall) {
            auto result = effect.callback->call();
            effect.cleanup = std::move(result);
        }
    }
    updatedStates.clear();
    dirty = false;
    hookCount = 0;
}

void ComponentContext::reconcileList(const SharedWidget &listHolder,
                                     std::unique_ptr<AmaraArray> arr,
                                     std::unique_ptr<StateWrapper> func) {
    auto holder = listHolder->as<ContainerWidget>();
    if (!holder->hasChildren()) {
        // Initial render case (unchanged)
        for (int i = 0; i < arr->size(); ++i) {
            const auto val = arr->getValue(i);
            auto result = func->call(val->getValue(), Value(i));
            if (!result->hasValue() || (result->getValue().isBool() && !result->getValue().asBool())) {
                continue;
            }
            auto newWidgetHolder = engine->getWidgetHolder(result);
            if (newWidgetHolder) {
                // Skip if null
                auto widget = newWidgetHolder->execute(engine);
                holder->addChild(widget);
            }
        }
    } else {
        auto &currentChildren = holder->children();
        const size_t newSize = arr->size();

        // Build map of existing keyed widgets
        std::unordered_map<std::string, SharedWidget> existingChildren;
        for (const auto &child: currentChildren) {
            if (child->key.hasKey()) {
                existingChildren[child->key.key] = child;
            }
        }

        std::vector<SharedWidget> newChildren;
        newChildren.reserve(newSize);
        unordered_set<std::string> usedKeys;

        // Reconcile new items
        for (int i = 0; i < newSize; ++i) {
            const auto val = arr->getValue(i);
            auto result = func->call(val->getValue(), Value(i));
            if (!result->hasValue() || (result->getValue().isBool() && !result->getValue().asBool())) {
                continue;
            }
            auto newWidgetHolder = engine->getWidgetHolder(result);
            auto newKey = newWidgetHolder->key();

            SharedWidget widget;
            if (newKey.hasKey() && existingChildren.count(newKey.key)) {
                // Reuse the existing widget with this key
                widget = existingChildren[newKey.key];
                widget->component()->_reconciliationStarted = true;
                widget = this->reconcileObject(widget, std::move(newWidgetHolder));
                widget->component()->_reconciliationStarted = false;
                widget->component()->hookCount = 0;
                usedKeys.insert(newKey.key);
            } else {
                // Create new widget
                widget = newWidgetHolder->execute(engine);
            }

            newChildren.push_back(widget);
        }

        // Update container children (only modify DOM if needed)
        holder->replaceChildren(std::move(newChildren));

        // Destroy any unused widgets
        for (const auto &[key, widget]: existingChildren) {
            if (!usedKeys.count(key)) {
                widget->resetPointer();
            }
        }
    }
}


std::shared_ptr<Widget> ComponentContext::reconcileObject(const std::shared_ptr<Widget> &old,
                                                          std::unique_ptr<WidgetHolder> newCaller) {
    auto &subComponent = old->component();
    subComponent->_reconciliationStarted = true;
    //To make the new state
    subComponent->_updateStates();
    //The perfect case.
    if (!old->is<ContainerWidget>()) {
        engine->plugComponent(
            subComponent);
        auto result = newCaller->execute(engine);
        engine->unplugComponent();
        subComponent->_reconciliationStarted = false;
        return result;
    }
    auto oldContainer = old->as<ContainerWidget>();
    auto oldReconcilingObject = subComponent->reconcilingObject;
    bool reconcilState = subComponent->_reconciliationStarted;
    subComponent->_reconciliationStarted = true;
    subComponent->reconcilingObject = oldContainer;
    engine->pushExistingComponent(subComponent);
    SharedWidget newWidget = newCaller->execute(engine);
    subComponent->reconcilingObject.reset();
    //To avoid
    subComponent->toBeUpdated.clear();
    //There is only one case that force us to reset the reconciliation flag which is adding dynamic children.
    subComponent->_reconciliationStarted = reconcilState;
    subComponent->dirty = false;
    subComponent->reconcilingObject = oldReconcilingObject;
    old->resetPointer();
    return newWidget;
}
