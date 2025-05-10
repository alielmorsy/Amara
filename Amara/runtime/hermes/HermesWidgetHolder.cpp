//
// Created by Ali Elmorsy on 4/5/2025.
//

#include "HermesWidgetHolder.h"
#include "../../ui/Widget.h"
#include "WidgetHostWrapper.h"
#include "Engine.h"

std::shared_ptr<Widget> HermesWidgetHolder::execute(IEngine *engine) {
    auto hermesProps = dynamic_cast<HermesPropMap *>(_props.get());
    if (isInternal) {
        assert(componentName.has_value() && "Component marked as internal but without component Name");

        auto arr = hermesProps->get("children").asObject(rt).asArray(rt);
        auto &c = hermesProps->getHermesValue();
        auto widget = engine->createComponent(componentName.value(), std::make_unique<HermesPropMap>(rt, Value(rt, c)));
        if (widget->is<TextWidget>()) {
            auto textWidget = widget->as<TextWidget>();
            for (int i = 0; i < arr.size(rt); ++i) {
                auto val = arr.getValueAtIndex(rt, i);
                std::string string;
                if (val.isObject()) {
                    auto wrapper = StateWrapper::create(rt, std::move(val));
                    if (wrapper->isStateVariable()) {
                        string = wrapper->getInternalValue()->getValue().asString(rt).utf8(rt);
                    } else {
                        string = val.asObject(rt).getProperty(rt, "toString").asObject(rt).asFunction(rt).call(rt).
                                asString(rt).utf8(rt);
                    }
                } else {
                    string = val.asString(rt).utf8(rt);
                }
                textWidget->addText(string);
            }
        } else if (widget->is<ContainerWidget>()) {
            auto container = widget->as<ContainerWidget>();
            for (int i = 0; i < arr.size(rt); ++i) {
                auto val = arr.getValueAtIndex(rt, i);
                auto ref = StateWrapper::create(rt, std::move(val));
                auto child = engine->getWidgetHolder(ref);
                auto childWidget = child->execute(engine);
                container->addChild(childWidget);
            }
        }
        if (key().hasKey()) {
            widget->key = key();
        }
        return widget;
    }
    auto &c = hermesProps->getHermesValue();
    const auto result = componentFunction->asObject(rt).asFunction(rt).call(rt,Value(rt, c));
    auto widget = result.asObject(rt).asHostObject<WidgetHostWrapper>(rt)->getNativeWidget();
    if (key().hasKey()) {
        widget->key = key();
    }
    return widget;
}

std::vector<std::unique_ptr<WidgetHolder> > HermesWidgetHolder::getChildren() {
    auto arr = _props->getArray("children");
    if (!arr) return {};
    std::vector<std::unique_ptr<WidgetHolder> > children;
    children.reserve(arr->size());
    for (int i = 0; i < arr->size(); ++i) {
        const auto val = arr->getValue(i);
        children.emplace_back(create(rt, val->getValueRef()));
    }

    return children;
}
