#include "ComponentContext.h"

#include <unordered_set>

#include "../runtime/WidgetHolder.h"
#include "../runtime/IEngine.h"
#include "Widget.h"
#include "../utils/ScopedTimer.h"

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
        if (state.object->equals(newValue.get())) {
            return;
        }
        //Notify parent if needed
        notifier();
        toBeUpdated[currentIndex] = std::move(newValue);
        dirty = true;
    };

    return {wrapper, std::move(setState)};
}

void ComponentContext::effect(StateWrapperRef fn, std::vector<StateWrapperRef> deps) {
    if (_reconciliationStarted) {
        ScopedTimer timer;
        if (!deps.empty()) {
            fn->call();
        }

        return;
    }
    //The initial register call
    auto result = fn->call();

    effects.emplace_back(std::move(fn), std::move(deps), std::move(result));
}

void ComponentContext::_updateStates() {
    for (auto &data: toBeUpdated) {
        states[data.first].object->setValue(data.second);
    }
}

void ComponentContext::update() {
    if (!dirty)
        return;

    _updateStates();

    for (auto &effect: effects) {
        if (effect.cleanup->hasValue()) {
            effect.cleanup->call();
        }
        auto &deps = effect.deps;
        if (deps.empty()) continue;
        auto needRecall = false;
        for (auto &dep: deps) {
            for (auto &pair: toBeUpdated) {
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
    toBeUpdated.clear();
    dirty = false;
    hookCount = false;
}

void ComponentContext::reconcileList(const SharedWidget &listHolder,
                                     std::unique_ptr<AmaraArray> arr,
                                     unique_ptr<StateWrapper> func) {
    auto holder = listHolder->as<ContainerWidget>();
    if (!holder->hasChildren()) {
        // Initial render case
        for (int i = 0; i < arr->size(); ++i) {
            const auto val = arr->getValue(i);
            auto result = func->call(val->getValue(), Value(i));
            auto newWidgetHolder = engine->getWidgetHolder(result);
            auto widget = newWidgetHolder->execute(engine);
            holder->addChild(widget);
        }
    } else {
        auto &currentChildren = holder->children();
        const int newSize = arr->size();

        // Build map of existing children by key
        std::unordered_map<std::string, SharedWidget> existingChildren;
        for (const auto &child: currentChildren) {
            if (child->key.hasKey()) {
                existingChildren[child->key.key] = child;
            }
        }

        std::vector<SharedWidget> newChildren;
        unordered_set<std::string> usedKeys;

        // Reconcile new items
        for (int i = 0; i < newSize; ++i) {
            const auto val = arr->getValue(i);
            auto result = func->call(val->getValue(), Value(i));
            auto newWidgetHolder = engine->getWidgetHolder(result);
            auto newKey = newWidgetHolder->key();

            SharedWidget widget;
            if (newKey.hasKey() && existingChildren.count(newKey.key)) {
                // Reuse the existing widget with this key
                widget = existingChildren[newKey.key];
                widget = this->reconcileObject(widget, std::move(newWidgetHolder));
                usedKeys.insert(newKey.key);
            } else {
                // Create new widget
                widget = newWidgetHolder->execute(engine);
            }

            newChildren.push_back(widget);
        }

        // Update container children (only modify DOM if needed)
        holder->replaceChildren(newChildren);

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
    ScopedTimer timer;
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

    subComponent->_reconciliationStarted = true;
    subComponent->reconcilingObject = oldContainer;
    engine->pushExistingComponent(subComponent);
    SharedWidget newWidget = newCaller->execute(engine);
    subComponent->_reconciliationStarted = false;
    subComponent->reconcilingObject.reset();
    //To avoid
    subComponent->dirty = false;
    old->resetPointer();
    return newWidget;
}
