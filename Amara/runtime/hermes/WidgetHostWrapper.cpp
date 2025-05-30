//
// Created by Ali Elmorsy on 3/31/2025.
//

#include "WidgetHostWrapper.h"
#include "HermesWidgetHolder.h"
#include "Engine.h"
#include "HermesArray.h"
#include "../utils/ScopedTimer.h"

static const char *CHILDREN_ID = "CHILDREN_SPECIAL_ID";

Value WidgetHostWrapper::get(Runtime &runtime, const PropNameID &propName) {
    auto name = propName.utf8(runtime);
    if (name == "addText") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return addText(rt, args, count);
            });
    }
    if (name == "addChild") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return addChild(rt, args, count);
            });
    }
    if (name == "insertChild") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return insertChild(rt, args, count);
            });
    } else if (name == "addStaticChild") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return addStaticChild(rt, args, count);
            });
    }
    if (name == "insertChildren") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return insertChildren(rt, args, count);
            });
    }
    if (name == "removeChildren") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return insertChildren(rt, args, count);
            });
    }
    if (name == "setChild") {
        static int i = 0;
        i += 1;
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return setChild(rt, args, count);
            });
    }
    if (name == "removeChild") {
        return Function::createFromHostFunction(
            runtime,
            propName,
            1, [this](Runtime &rt, const Value &thisValue, const Value *args, const size_t count) {
                return removeChild(rt, args, count);
            });
    }
    return Value::undefined();
}

void WidgetHostWrapper::set(Runtime &runtime, const PropNameID &name, const Value &value) {
}

Value WidgetHostWrapper::addText(Runtime &rt, const Value *args, const size_t count) {
    if (count != 1 || !args[0].isString()) {
        throw JSError(rt, "addText function accept one argument only and its type must be string");
    }
    auto widget = nativeWidget.lock();
    auto textWidget = widget->as<TextWidget>();
    if (!textWidget) {
        throw JSError(rt, "You cannot use addText over a non text widget");
    }
    textWidget->addText(args[0].asString(rt).utf8(rt));
    return Value::undefined();
}

Value WidgetHostWrapper::addChild(Runtime &rt, const Value *args, const size_t count) {
    if (count != 1 || !args[0].isObject()) {
        throw JSError(rt, "addChild function accept one argument only and its type must be an object");
    }
    auto widget = nativeWidget.lock();
    auto containerWidget = widget->as<ContainerWidget>();
    if (!containerWidget) {
        throw JSError(rt, "You cannot use addChild over a non container widget");
    }
    Object obj = args[0].asObject(rt);
    // I am pretty sure we won't need that but just in case
    if (!obj.isHostObject(rt) && obj.hasProperty(rt, "$$internalComponent")) {
        throw JSError(rt, "You cannot add child this way anymore");

    }
    auto child = obj.asHostObject<WidgetHostWrapper>(rt);
    auto childWidget = child->getNativeWidget();
    containerWidget->addChild(childWidget);
    return Value::undefined();
}

Value WidgetHostWrapper::addStaticChild(Runtime &rt, const Value *args, size_t count) {
    if (count != 1 || !args[0].isObject()) {
        throw JSError(rt, "addChild function accept one argument only and its type must be an object");
    }
    auto widget = nativeWidget.lock();
    auto containerWidget = widget->as<ContainerWidget>();
    if (!containerWidget) {
        throw JSError(rt, "You cannot use addChild over a non container widget");
    }
    auto holder = engine->getWidgetHolder(args[0]);

    containerWidget->addStaticChild(engine, std::move(holder));
    return Value::undefined();
}

Value WidgetHostWrapper::insertChild(Runtime &rt, const Value *args, size_t count) {
    if (count != 2 || !args[0].isString()) {
        throw JSError(rt, "insertChild function accept two argument only and the first argument must be a string ID");
    }
    auto widget = nativeWidget.lock();
    auto id = args[0].asString(rt).utf8(rt);
    if (widget->is<TextWidget>()) {
        auto &arg = args[1];
        std::string text;
        if (arg.isString()) {
            text = arg.asString(rt).utf8(rt);
        } else {
            text = arg.asObject(rt).getProperty(rt, "toString").asObject(rt).asFunction(rt).call(rt).asString(rt).
                    utf8(rt);
        }
        widget->as<TextWidget>()->insertChild(id, text);
        return Value::undefined();
    }
    auto containerWidget = widget->as<ContainerWidget>();
    if (!containerWidget) {
        throw JSError(rt, "You cannot use insertChild over a non container widget");
    }
    if (args[1].isObject()) {
        auto obj = args[1].asObject(rt);
        if (obj.isHostObject<WidgetHostWrapper>(rt)) {
            auto newWidget = obj.asHostObject<WidgetHostWrapper>(rt)->nativeWidget.lock();
            if (!newWidget->is<HolderWidget>()) {
                throw JSError(rt, "You cannot use insertChild non static child or a holder");
            }
            auto holder = newWidget->as<HolderWidget>();
            containerWidget->insertChild(std::move(id), holder->child);
        } else {
            auto holder = engine->getWidgetHolder(args[1]);
            containerWidget->insertChild(engine, std::move(id), std::move(holder));
        }
    } else {
        //State variable
    }

    return Value::undefined();
}

Value WidgetHostWrapper::insertChildren(Runtime &rt, const Value *args, size_t count) {
    auto &children = args[0];
    auto widget = nativeWidget.lock();
    assert(widget->is<ContainerWidget>() && "Widget is not a container widget");
    auto containerWidget = widget->as<ContainerWidget>();
    auto arr = std::make_unique<HermesArray>(rt, Value(rt, children));
    auto emptyProps = Value();
    std::string type = "component";
    auto holder = engine->createComponent(type, std::make_unique<HermesPropMap>(rt, Object(rt)));
    auto holderContainer = holder->as<ContainerWidget>();
    for (int i = 0; i < arr->size(); ++i) {
        auto val = arr->getValue(i);
        auto w = engine->getWidgetHolder(val)->execute(engine);
        holderContainer->addChild(w);
    }
    containerWidget->insertChild(CHILDREN_ID, holder);
    return Value::undefined();
}

Value WidgetHostWrapper::removeChildren(Runtime &rt, const Value *args, size_t count) {
    auto widget = nativeWidget.lock();
    assert(widget->is<ContainerWidget>() && "Widget is not a container widget");
    auto containerWidget = widget->as<ContainerWidget>();
    std::string id = CHILDREN_ID;
    containerWidget->removeChild(id);
}

Value WidgetHostWrapper::setChild(Runtime &rt, const Value *args, size_t count) {
    auto widget = nativeWidget.lock();
    assert(widget->is<HolderWidget>() && "Widget is not a container widget");
    auto holderWidget = widget->as<HolderWidget>();
    auto widgetHolder = engine->getWidgetHolder(args[0]);
    holderWidget->setChild(engine, std::move(widgetHolder));
    return Value::undefined();
}

Value WidgetHostWrapper::removeChild(Runtime &rt, const Value *args, size_t count) {
    auto widget = nativeWidget.lock()->as<ContainerWidget>();

    auto id = args[0].asString(rt).utf8(rt);
    widget->removeChild(id);
    return Value::undefined();
}

SharedWidget WidgetHostWrapper::getNativeWidget() const {
    return nativeWidget.lock();
}
