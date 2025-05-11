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
    updating = true;


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
    updating = false;
}


std::shared_ptr<Widget> ComponentContext::reconcileObject(const std::shared_ptr<Widget> &old,
                                                          std::unique_ptr<WidgetHolder> &newCaller) {
    auto &subComponent = old->component();
    //bool sameComponent = subComponent.get() == this;

    auto componentName = newCaller->getComponentName();


    auto &newProps = newCaller->props();
    if (newCaller->isComponent()) {
        if (newCaller->sameComponent(subComponent->componentObject)) {
            subComponent->_reconciliationStarted = true;
            subComponent->_updateStates();

            engine->pushExistingComponent(subComponent);
            auto result = newCaller->execute(engine);
            subComponent->_reconciliationStarted = false;
            return result;
        }
        //TODO: We need unmounting here
        return newCaller->execute(engine);
    }
    if (componentName == "div" && old->is<ContainerWidget>()) {
        subComponent->_reconciliationStarted = true;
        engine->compareProps(old->propMap, newProps);
        auto children = newCaller->getChildren();
        reconcileWidgetHolders(old->as<ContainerWidget>(), std::move(children));
        subComponent->_reconciliationStarted = false;
        return old;
    }
    if (componentName == "text" && old->is<TextWidget>()) {
        //I am pretty sure we need a new way of handling this
        engine->compareProps(old->propMap, newProps);
        auto newWidget = newCaller->execute(engine)->as<TextWidget>();
        auto textWidget = old->as<TextWidget>();
        textWidget->replaceChildren(newWidget->children());
        newWidget->resetPointer();
        return old;
    }
    return newCaller->execute(engine);
}

/**
 * Original reconcileList function that maps array items and a function to widget holders
 * and passes them to the generalized reconciler
 */
void ComponentContext::reconcileList(const SharedWidget &listHolder,
                                     std::unique_ptr<AmaraArray> arr,
                                     std::unique_ptr<StateWrapper> func) {
    // Convert array items to widget holders
    std::vector<std::unique_ptr<WidgetHolder> > widgetHolders;
    widgetHolders.reserve(arr->size());

    for (int i = 0; i < arr->size(); ++i) {
        const auto val = arr->getValue(i);
        auto result = func->call(val->getValue(), Value(i));

        // Skip if result is falsy
        if (!result->hasValue() || (result->getValue().isBool() && !result->getValue().asBool())) {
            continue;
        }

        auto widgetHolder = engine->getWidgetHolder(result);
        if (widgetHolder) {
            widgetHolders.push_back(std::move(widgetHolder));
        }
    }

    // Call the generalized reconciliation function
    reconcileWidgetHolders(listHolder->as<ContainerWidget>(), std::move(widgetHolders));
}

/**
 * Generalized function that reconciles a container with a vector of widget holders
 */
void ComponentContext::reconcileWidgetHolders(const std::shared_ptr<ContainerWidget> &listHolder,
                                              std::vector<std::unique_ptr<WidgetHolder> > widgetHolders) {
    auto holder = listHolder->as<ContainerWidget>();

    // Initial render case
    if (!holder->hasChildren()) {
        for (const auto &widgetHolder: widgetHolders) {
            if (!widgetHolder) continue;
            auto widget = widgetHolder->execute(engine);
            if (widget) {
                holder->addChild(widget);
            }
        }
        return;
    }

    // Reconciliation case
    auto currentChildren = holder->children();
    const size_t newSize = widgetHolders.size();

    // Build map of existing keyed widgets with their original indices
    std::unordered_map<std::string, std::pair<SharedWidget, size_t> > existingChildren;
    for (size_t i = 0; i < currentChildren.size(); ++i) {
        if (currentChildren[i]->key.hasKey()) {
            existingChildren[currentChildren[i]->key.key] = {currentChildren[i], i};
        }
    }

    // Prepare new children list and tracking structures
    std::vector<SharedWidget> newChildren;
    newChildren.reserve(newSize);
    std::unordered_set<std::string> usedKeys;
    std::vector<std::pair<int, size_t> > oldIndices; // (new index, old index)

    // Process each new widget holder
    for (int i = 0; i < newSize; ++i) {
        auto &widgetHolder = widgetHolders[i];
        if (!widgetHolder) continue;

        auto newKey = widgetHolder->key();
        SharedWidget widget;

        if ((newKey.hasKey() && existingChildren.count(newKey.key))) {
            // Reuse existing widget
            widget = existingChildren[newKey.key].first;
            widget->component()->_reconciliationStarted = true;
            widget = this->reconcileObject(widget, widgetHolder);
            widget->component()->_reconciliationStarted = false;
            widget->component()->hookCount = 0;
            usedKeys.insert(newKey.key);
            oldIndices.emplace_back(i, existingChildren[newKey.key].second);
        } else if (!newKey.hasKey() && i < currentChildren.size() && !currentChildren[i]->key.hasKey()) {
            widget = currentChildren[i];
            widget->component()->_reconciliationStarted = true;
            widget = this->reconcileObject(widget, widgetHolder);
            widget->component()->_reconciliationStarted = false;
            widget->component()->hookCount = 0;
            oldIndices.emplace_back(i, i);
        } else {
            // Create new widget
            widget = widgetHolder->execute(engine);
            if (newKey.hasKey()) {
                oldIndices.emplace_back(i, -1); // New widget, no old index
            }
        }

        if (widget) {
            newChildren.push_back(widget);
        }
    }

    // Compute Longest Increasing Subsequence to find widgets that maintain relative order
    std::vector<int> lis;
    if (!oldIndices.empty()) {
        for (auto &p: oldIndices) {
            auto it = std::lower_bound(lis.begin(), lis.end(), p.second);
            if (it == lis.end()) {
                lis.push_back(p.second);
            } else {
                *it = p.second;
            }
        }
    }

    // Apply minimal updates to container
    std::vector<SharedWidget> tempChildren = currentChildren;
    size_t currentIdx = 0, newIdx = 0;

    while (newIdx < newChildren.size()) {
        bool isInLIS = std::find(lis.begin(), lis.end(), newIdx) != lis.end();

        if (currentIdx < tempChildren.size() && isInLIS &&
            tempChildren[currentIdx] == newChildren[newIdx]) {
            // Widget is in LIS and at correct position - keep it
            ++currentIdx;
            ++newIdx;
        } else {
            // Widget needs to be inserted or moved
            bool found = false;
            for (size_t j = currentIdx; j < tempChildren.size(); ++j) {
                if (tempChildren[j] == newChildren[newIdx]) {
                    // Move existing widget to the right position
                    holder->replaceChild(j, newChildren[newIdx]);
                    tempChildren.erase(tempChildren.begin() + j);
                    tempChildren.insert(tempChildren.begin() + newIdx, newChildren[newIdx]);
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Insert new widget
                holder->insertChild(newIdx, newChildren[newIdx]);
                tempChildren.insert(tempChildren.begin() + newIdx, newChildren[newIdx]);
            }
            ++currentIdx;
            ++newIdx;
        }
    }

    // Remove any remaining children not in new list
    while (currentIdx < tempChildren.size()) {
        holder->removeChild(currentIdx);
        tempChildren.erase(tempChildren.begin() + currentIdx);
    }

    // Destroy unused widgets
    for (const auto &[key, widgetPair]: existingChildren) {
        if (usedKeys.find(key) == usedKeys.end()) {
            if (widgetPair.first) {
                widgetPair.first->resetPointer();
            }
        }
    }
}
