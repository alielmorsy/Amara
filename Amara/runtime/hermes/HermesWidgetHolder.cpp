//
// Created by Ali Elmorsy on 4/5/2025.
//

#include "HermesWidgetHolder.h"
#include "../../ui/Widget.h"
#include "WidgetHostWrapper.h"
#include "Engine.h"

std::shared_ptr<Widget> HermesWidgetHolder::execute(IEngine *engine) {
    if (isInternal) {
        assert(componentName.has_value() && "Component marked as internal but without component Name");
        auto arr = props->asObject(rt).getProperty(rt, "children").asObject(rt).asArray(rt);
        auto widget = engine->createComponent(componentName.value(), *props);
        if (widget->is<TextWidget>()) {
            auto textWidget = widget->as<TextWidget>();
            for (int i = 0; i < arr.size(rt); ++i) {
                auto val = arr.getValueAtIndex(rt, i);
                textWidget->addText(val.asString(rt).utf8(rt));
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
    const auto result = componentFunction->asObject(rt).asFunction(rt).call(rt, *props);
    auto widget = result.asObject(rt).asHostObject<WidgetHostWrapper>(rt)->getNativeWidget();
    if (key().hasKey()) {
        widget->key = key();
    }
    return widget;
}
