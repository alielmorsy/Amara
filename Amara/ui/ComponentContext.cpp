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
                auto widget = newWidgetHolder->execute(engine);
                holder->addChild(widget);
            }
        }
    } else {
        auto currentChildren = holder->children();
        const size_t newSize = arr->size();

        // Build map of existing keyed widgets with their original indices
        std::unordered_map<std::string, std::pair<SharedWidget, size_t> > existingChildren;
        for (size_t i = 0; i < currentChildren.size(); ++i) {
            if (currentChildren[i]->key.hasKey()) {
                existingChildren[currentChildren[i]->key.key] = {currentChildren[i], i};
            }
        }

        std::vector<SharedWidget> newChildren;
        newChildren.reserve(newSize);
        std::unordered_set<std::string> usedKeys;
        std::vector<std::pair<int, size_t> > oldIndices; // (new index, old index)

        // Reconcile new items and track positions
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
                // Reuse existing widget
                widget = existingChildren[newKey.key].first;
                widget->component()->_reconciliationStarted = true;
                widget = this->reconcileObject(widget, std::move(newWidgetHolder));
                widget->component()->_reconciliationStarted = false;
                widget->component()->hookCount = 0;
                usedKeys.insert(newKey.key);
                oldIndices.emplace_back(i, existingChildren[newKey.key].second);
            } else {
                // Create new widget
                widget = newWidgetHolder->execute(engine);
                if (newKey.hasKey()) {
                    oldIndices.emplace_back(i, -1); // New widget, no old index
                }
            }
            newChildren.push_back(widget);
        }

        // Compute LIS to find widgets that maintain relative order
        std::vector<int> lis;
        if (!oldIndices.empty()) {
            std::vector<int> dp(oldIndices.size(), 1);
            std::vector<int> prev(oldIndices.size(), -1);
            size_t maxLen = 1, endIdx = 0;

            for (size_t i = 1; i < oldIndices.size(); ++i) {
                for (size_t j = 0; j < i; ++j) {
                    if (oldIndices[i].second > oldIndices[j].second && dp[j] + 1 > dp[i]) {
                        dp[i] = dp[j] + 1;
                        prev[i] = j;
                        if (dp[i] > maxLen) {
                            maxLen = dp[i];
                            endIdx = i;
                        }
                    }
                }
            }

            // Reconstruct LIS
            while (endIdx != -1) {
                lis.push_back(oldIndices[endIdx].first);
                endIdx = prev[endIdx];
            }
            std::reverse(lis.begin(), lis.end());
        }

        // Apply minimal updates to container
        std::vector<SharedWidget> tempChildren = currentChildren;
        size_t currentIdx = 0, newIdx = 0;

        while (newIdx < newChildren.size()) {
            //This check assumes that any unchanged widget will not change
            if (currentIdx < tempChildren.size() &&
                std::find(lis.begin(), lis.end(), newIdx) != lis.end() &&
                tempChildren[currentIdx] == newChildren[newIdx]) {
                // Widget is in LIS and at correct position
                ++currentIdx;
                ++newIdx;
            } else {
                // Widget needs to be inserted or moved
                bool found = false;
                for (size_t j = currentIdx; j < tempChildren.size(); ++j) {
                    if (tempChildren[j] == newChildren[newIdx]) {
                        //        holder->moveChild(j, newIdx);
                        tempChildren.erase(tempChildren.begin() + j);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    holder->replaceChild(newIdx, newChildren[newIdx]);
                }
                ++newIdx;
                if (currentIdx < tempChildren.size()) ++currentIdx;
            }
        }

        // Remove any remaining children not in new list
        while (currentIdx < tempChildren.size()) {
            holder->removeChild(currentIdx);
            tempChildren.erase(tempChildren.begin() + currentIdx);
        }

        // Destroy unused widgets
        for (const auto &[key, widgetPair]: existingChildren) {
            if (!usedKeys.count(key)) {
                widgetPair.first->resetPointer();
            }
        }
    }
}


std::shared_ptr<Widget> ComponentContext::reconcileObject(const std::shared_ptr<Widget> &old,
                                                          std::unique_ptr<WidgetHolder> newCaller) {
    auto &subComponent = old->component();
    subComponent->_reconciliationStarted = true;
    subComponent->_updateStates();

    if (!old->is<ContainerWidget>()) {
        engine->plugComponent(subComponent);
        auto result = newCaller->execute(engine);
        engine->unplugComponent();
        subComponent->_reconciliationStarted = false;
        return result;
    }

    auto oldContainer = old->as<ContainerWidget>();
    subComponent->reconcilingObject = oldContainer;
    engine->pushExistingComponent(subComponent);
    SharedWidget newWidget = newCaller->execute(engine);
    subComponent->reconcilingObject.reset();
    subComponent->_reconciliationStarted = false;
    subComponent->dirty = false;

    if (newWidget->is<ContainerWidget>()) {
        auto newContainer = newWidget->as<ContainerWidget>();
        reconcileChildren(oldContainer, newContainer);
    }

    old->resetPointer();
    return newWidget;
}

void ComponentContext::reconcileChildren(std::shared_ptr<ContainerWidget> &oldContainer,
                                         shared_ptr<ContainerWidget> &newContainer) {
    return;
    auto &oldChildren = oldContainer->children();
    auto &newChildren = newContainer->children();
    size_t maxSize = std::max(oldChildren.size(), newChildren.size());

    for (size_t i = 0; i < maxSize; ++i) {
        if (i >= oldChildren.size()) {
            oldChildren.push_back(newChildren[i]);
        } else if (i >= newChildren.size()) {
            oldChildren.resize(newChildren.size());
            break;
        } else {
            oldChildren[i] = newChildren[i];
        }
    }
}
