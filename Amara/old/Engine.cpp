//
// Created by Ali Elmorsy on 3/26/2025.
//

#include "Engine.h"

#include <iostream>
#include <ranges>
#include <thread>
#include <typeindex>
#include <hermes/hermes.h>
#include "Widget.h"

class WidgetPool {
public:
    template<typename T, typename... Args>
    std::unique_ptr<T> acquire(Args &&... args) {
        static_assert(std::is_base_of_v<PrimitiveWidget, T>,
                      "WidgetPool can only manage PrimitiveWidget derivatives");

        if (auto &pool = getPool(typeid(T)); !pool.empty()) {
            auto widget = std::move(pool.back());
            pool.pop_back();
            auto *typedWidget = static_cast<T *>(widget.release());
            typedWidget->reset(std::forward<Args>(args)...); // Reset widget state
            return std::unique_ptr<T>(typedWidget);
        }
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    void release(std::unique_ptr<PrimitiveWidget> widget) {
        if (!widget) return;

        // Clear widget state before returning to pool
        widget->children.clear();
        widget->elementIndex.reset();

        auto &pool = getPool(typeid(*widget));
        pool.push_back(std::move(widget));
    }

    void clear() {
        pools_.clear();
    }

private:
    std::map<std::type_index, std::vector<std::unique_ptr<PrimitiveWidget> > > pools_;

    std::vector<std::unique_ptr<PrimitiveWidget> > &getPool(const std::type_index &type) {
        auto it = pools_.find(type);
        if (it == pools_.end()) {
            it = pools_.emplace(type, std::vector<std::unique_ptr<PrimitiveWidget> >()).first;
        }
        return it->second;
    }
};

Engine::~Engine() {
    elements.clear();
}

WidgetPool widgetPool_;

int Engine::createElementTree(Runtime &rt, const Value &type, const Value &props, const Value &key) {
    int index = counter++;
    std::shared_ptr<BaseElement> element;
    auto propsRef = std::make_shared<Value>(rt, props);
    if (type.isString()) {
        auto primitive = type.getString(rt).utf8(rt);
        element = std::make_shared<PrimitiveElement>(primitive, propsRef);
    } else {
        auto func = std::make_shared<Value>(rt, type);

        element = std::make_shared<ComponentElement>(func, propsRef);
        int x = 0;
    }
    if (!key.isUndefined()) {
        if (key.isString()) {
            auto keyStr = key.getString(rt).utf8(rt);
            element->key = Key(keyStr);
        } else {
            element->key = Key(key.asNumber());
        }
    }
    elements[index] = element;
    return index;
}

void Engine::render(Runtime &rt, int index = 0, bool initial) {
    auto mainElement = elements[index];
    auto start = std::chrono::high_resolution_clock::now();
    rootWidget = buildWidget(rt, index, std::move(rootWidget));
    previousElements = std::move(elements); // Save for next render
    elements.clear(); // Prepare for next render
    counter = 0;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start);
    std::cout << "Time taken: " << duration.count() << " microseconds" <<
            std::endl;
    int i = 0;
    while (initial) {
        elements[index] = mainElement;
        if (!updatedIndices.empty()) {
            updatedIndices.clear();
            // partialUpdate(rt);
            render(rt, index, false);
        } else {
            break;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        i++;
    }
    // if (renderRquested) {
    //     renderRquested = false;
    //     elements[index] = mainElement;
    //     render(rt, index);
    // }
}

void Engine::partialUpdate(Runtime &rt) {
    for (auto index: updatedIndices) {
    }
}

std::unique_ptr<Widget> Engine::buildWidget(Runtime &rt, int index, std::unique_ptr<Widget> prevWidget) {
    auto &element = elements[index];

    if (element->isComponent()) {
        // Handle components
        currentComponentIndex = index;
        auto componentElement = element->as<ComponentElement>();
        auto func = componentElement->componentFunction();
        auto props = componentElement->props;
        auto result = func->asObject(rt).asFunction(rt).call(rt, *props);
        int renderedIndex = result.asNumber();
        componentStates[currentComponentIndex].hookIndex = 0;
        currentComponentIndex = -1;

        // Pass the previous rendered widget (if any) to reconcile the rendered output
        return buildWidget(rt, renderedIndex, std::move(prevWidget));
    }

    // Handle primitive elements
    auto primitiveElement = element->as<PrimitiveElement>();
    auto &type = primitiveElement->primitive();
    auto props = primitiveElement->props;
    std::unique_ptr<PrimitiveWidget> widget;

    // Check if we can reuse the previous widget
    if (prevWidget && prevWidget->is<PrimitiveWidget>() &&
        prevWidget->as<PrimitiveWidget>()->type == type) {
        widget = std::unique_ptr<PrimitiveWidget>(static_cast<PrimitiveWidget *>(prevWidget.release()));
    } else {
        // Release mismatched widget to pool if it's a PrimitiveWidget
        if (prevWidget && prevWidget->is<PrimitiveWidget>()) {
            widgetPool_.release(std::unique_ptr<PrimitiveWidget>(
                static_cast<PrimitiveWidget *>(prevWidget.release())
            ));
        }
        // Acquire new widget from pool
        widget = widgetPool_.acquire<PrimitiveWidget>(type);
    }

    // Reconcile children
    std::vector<std::unique_ptr<Widget> > newChildren;
    if (props->isObject() && props->asObject(rt).hasProperty(rt, "children")) {
        auto childrenVal = props->asObject(rt).getProperty(rt, "children");
        if (childrenVal.isString()) {
            // For TextWidget, we could also implement pooling if needed
            std::unique_ptr<Widget> child = std::make_unique<TextWidget>(childrenVal.asString(rt).utf8(rt));
            newChildren.push_back(std::move(child));
            goto end;
        }
        auto object = childrenVal.asObject(rt);

        if (object.isArray(rt)) {
            auto childrenArr = object.asArray(rt);
            size_t length = childrenArr.size(rt);

            // Prepare previous children for reconciliation
            auto &prevChildren = widget->children;
            std::unordered_map<Key, std::unique_ptr<Widget> > keyedPrevChildren;
            std::vector<std::unique_ptr<Widget> > unkeyedPrevChildren;

            for (auto &child: prevChildren) {
                if (child->elementIndex.has_value() &&
                    previousElements.contains(child->elementIndex.value())) {
                    auto &childElem = previousElements[child->elementIndex.value()];
                    if (childElem->key.has_value()) {
                        keyedPrevChildren[childElem->key.value()] = std::move(child);
                    } else {
                        unkeyedPrevChildren.push_back(std::move(child));
                    }
                } else {
                    unkeyedPrevChildren.push_back(std::move(child));
                }
            }

            // Build new children, reusing where possible
            size_t i = 0;
            for (; i < length; ++i) {
                auto childVal = childrenArr.getValueAtIndex(rt, i);
                if (childVal.isNumber()) {
                    int childIndex = childVal.asNumber();
                    auto &childElem = elements[childIndex];
                    std::unique_ptr<Widget> prevChild;

                    if (childElem->key.has_value()) {
                        auto it = keyedPrevChildren.find(childElem->key.value());
                        if (it != keyedPrevChildren.end()) {
                            prevChild = std::move(it->second);
                            keyedPrevChildren.erase(it);
                        }
                    } else if (!unkeyedPrevChildren.empty()) {
                        prevChild = std::move(unkeyedPrevChildren.front());
                        unkeyedPrevChildren.erase(unkeyedPrevChildren.begin());
                    }

                    auto childWidget = buildWidget(rt, childIndex, std::move(prevChild));
                    newChildren.push_back(std::move(childWidget));
                }
            }

            // Release remaining unused children to the pool
            for (auto &child: keyedPrevChildren | views::values) {
                if (child->is<PrimitiveWidget>()) {
                    elements.erase(child->elementIndex.value());
                    widgetPool_.release(std::unique_ptr<PrimitiveWidget>(
                            static_cast<PrimitiveWidget *>(child.release()))
                    );
                }
            }
            for (auto &child: unkeyedPrevChildren) {
                if (child->is<PrimitiveWidget>()) {
                    elements.erase(child->elementIndex.value());
                    widgetPool_.release(std::unique_ptr<PrimitiveWidget>(
                            static_cast<PrimitiveWidget *>(child.release()))
                    );
                }
            }
        }
    }

end:
    widget->children = std::move(newChildren);
    widget->elementIndex = index;
    return widget;
}

Value Engine::useStateImpl(
    Runtime &rt,
    const Value &thisValue,
    const Value *args,
    size_t count
) {
    if (currentComponentIndex == -1) {
        throw std::runtime_error("useState called outside a component");
    }
    if (count < 1) {
        throw std::runtime_error("useState requires an initial value");
    }

    auto &state = componentStates[currentComponentIndex];
    size_t index = state.hookIndex++;

    // Initialize state if this is the first call for this hook
    if (index >= state.states.size()) {
        state.states.emplace_back(std::make_shared<Value>(rt, args[0]));
    }
    auto currentValue = state.states[index];
    if (args[0].isString()) {
        cout << currentValue->getString(rt).utf8(rt) << endl;
    }
    // Define the setter function
    auto setter = [this, index](
        facebook::jsi::Runtime &rt,
        const facebook::jsi::Value &thisValue,
        const facebook::jsi::Value *args,
        size_t count
    ) -> facebook::jsi::Value {
        if (count < 1) {
            throw std::runtime_error("setState requires a new value");
        }
        auto &state = componentStates[currentComponentIndex];
        auto &previousValue = *state.states[index];
        if (Value::strictEquals(rt, previousValue, args[0])) {
            return facebook::jsi::Value::undefined();;
        }
        state.states[index] = std::make_shared<Value>(rt, args[0]);
        updatedIndices.push_back(currentComponentIndex);

        return facebook::jsi::Value::undefined();
    };

    // Create return array: [currentValue, setState]
    auto array = facebook::jsi::Array::createWithElements(rt, *currentValue,
                                                          facebook::jsi::Function::createFromHostFunction(
                                                              rt,
                                                              PropNameID::forAscii(rt, "setState"),
                                                              1,
                                                              setter
                                                          )
    );

    return array;
}
