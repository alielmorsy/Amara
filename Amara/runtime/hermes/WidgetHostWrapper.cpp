//
// Created by Ali Elmorsy on 3/31/2025.
//

#include "WidgetHostWrapper.h"
#include "HermesWidgetHolder.h"
#include "Engine.h"
#include "HermesArray.h"
#include "../utils/ScopedTimer.h"

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
        if (obj.getProperty(rt, "$$internalComponent").asBool()) return Value::undefined();
        auto func = obj.getProperty(rt, "component");
        auto props = obj.getProperty(rt, "props");
        auto holder = std::make_unique<HermesWidgetHolder>(rt, std::make_unique<Value>(rt, func),
                                                           std::make_unique<Value>(rt, props));
        containerWidget->addStaticChild(engine, std::move(holder));
        return Value::undefined();
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
    auto holder = engine->getWidgetHolder(Value(rt, args[0]));

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
        auto holder = engine->getWidgetHolder(Value(rt, args[1]));

        containerWidget->insertChild(engine, std::move(id), std::move(holder));
        int x=0;
    } else {
        //State variable
    }

    return Value::undefined();
}

Value WidgetHostWrapper::insertChildren(Runtime &rt, const Value *args, size_t count) {
    auto &children = args[0];
    auto widget = nativeWidget.lock();
    assert(widget->is<ContainerWidget>() && "Width is not a container widget");
    auto containerWidget = widget->as<ContainerWidget>();
    auto arr = std::make_unique<HermesArray>(rt, Value(rt, children));
    auto emptyProps = Value();
    std::string type = "component";
    auto holder = engine->createComponent(type, Object(rt));
    auto holderContainer = holder->as<ContainerWidget>();
    for (int i = 0; i < arr->size(); ++i) {
        auto val = arr->getValue(i);
        auto w = engine->getWidgetHolder(val)->execute(engine);
        holderContainer->addChild(w);
    }
    containerWidget->addChild(holder);
    return Value::undefined();
}

SharedWidget WidgetHostWrapper::getNativeWidget() const {
    return nativeWidget.lock();
}
