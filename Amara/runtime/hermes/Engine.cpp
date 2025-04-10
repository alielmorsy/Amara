//
// Created by Ali Elmorsy on 3/31/2025.
//

#include "Engine.h"

#include <iostream>

#include "WidgetHostWrapper.h"
#include <hermes/hermes.h>

#include "HermesArray.h"
#include "HermesPropMap.h"
#include "HermesWidgetHolder.h"
#include "../../utils/ScopedTimer.h"

void HermesEngine::beginComponentImpl() {
    if (!_started) {
        throw JSINativeException("You cannot call components directly. Kindly use the render API");
    }
    if (!componentContextFactory.empty()) {
        auto val = componentContextFactory.top();
        contextStack.emplace(val);
        componentContextFactory.pop();
        return;
    }
    contextStack.emplace(std::make_shared<ComponentContext>(this));
}

void HermesEngine::endComponentImpl() {
    if (contextStack.empty()) {
        throw JSINativeException("You cannot call components directly. Kindly use the render API");
    }
    auto &context = contextStack.top();
    contextStack.pop();
}


void HermesEngine::componentEffectImpl(Value fn, const Value &deps) {
    if (contextStack.empty()) {
        throw JSINativeException("You cannot call components directly. Kindly use the render API");
    }
    auto &context = contextStack.top();

    std::vector<StateWrapperRef> depsVector;
    if (!deps.isUndefined()) {
        auto arr = deps.asObject(*runtime).asArray(*runtime);
        auto size = arr.size(*runtime);
        depsVector.reserve(size);
        for (int i = 0; i < size; ++i) {
            depsVector.emplace_back(StateWrapper::create(*runtime, arr.getValueAtIndex(*runtime, i)));
        }
    }

    context->effect(StateWrapper::create(*runtime, std::move(fn)), std::move(depsVector));
}


Value HermesEngine::useStateImpl(const Value &value) {
    if (contextStack.empty()) {
        throw JSINativeException("You cannot call components directly. Kindly use the render API");
    }

    auto &rt = *runtime;
    auto createElementRef = rt.global().getProperty(rt, "createRef").asObject(rt).asFunction(rt);


    auto createStateWrapper = [&](const Value &val) -> std::unique_ptr<StateWrapper> {
        if (val.isObject()) {
            auto obj = val.asObject(rt);
            if (obj.isFunction(rt)) {
                auto resolvedValue = obj.asFunction(rt).call(rt);
                auto proxy = createElementRef.call(rt, resolvedValue);
                return StateWrapper::create(rt, std::move(proxy));
            }
        }
        auto proxy = createElementRef.call(rt, val);
        return StateWrapper::create(rt, std::move(proxy));
    };

    auto wrapper = createStateWrapper(value);
    auto &context = contextStack.top();

    auto [stateValue, func] = context->useState(std::move(wrapper), [this, context=std::shared_ptr(context)]() {
        componentsToBeUpdated.emplace_back(context);
    });


    auto setter = [this,func=std::move(func)](Runtime &rt, const Value &thisValue, const Value *args, size_t count) {
        if (count != 1) {
            throw JSINativeException("setState requires exactly one argument");
        }
        auto newWrapper = StateWrapper::create(rt, Value(rt, args[0]));
        func(std::move(newWrapper));
        return Value::undefined();
    };

    return Array::createWithElements(
        rt,
        stateValue->getValue(),
        Function::createFromHostFunction(rt, PropNameID::forAscii(rt, "setState"), 1, setter)
    );
}

std::shared_ptr<Widget> HermesEngine::createComponent(std::string &type,
                                                      const Value &props) {
    auto propsMap = std::make_unique<HermesPropMap>(*runtime,
                                                    std::make_shared<Value>(*runtime, props));
    SharedWidget widget;
    if (type == "component" || type == "div") {
        widget = pool.allocate<ContainerWidget>(std::move(propsMap), contextStack.top());
    } else if (type == "text" || type == "h1" || type == "h2") {
        widget = pool.allocate<TextWidget>(std::move(propsMap), contextStack.top());
    } else if (type == "image") {
        widget = pool.allocate<ImageWidget>(std::move(propsMap), contextStack.top());
    } else if (type == "button") {
        widget = pool.allocate<ButtonWidget>(std::move(propsMap), contextStack.top());
    } else {
        throw JSError(*runtime, "Unknown component type: " + type);
    }
    contextStack.top()->widgets.push_back(widget);
    return widget;
}

void HermesEngine::render(const Value &value) {
    ScopedTimer timer;
    _started = true;
    auto &rt = *runtime;
    auto func = value.asObject(rt).asFunction(rt);

    const auto result = func.call(rt);

    if (!result.isObject()) {
        throw JSINativeException("Your initial function did something wrong");
    }

    rootWidget = std::move(result.asObject(rt).asHostObject<WidgetHostWrapper>(rt)->getNativeWidget());

    std::sort(componentsToBeUpdated.begin(), componentsToBeUpdated.end(),
              [](const std::shared_ptr<ComponentContext> &first, const std::shared_ptr<ComponentContext> &second) {
                  return first->index() < second->index();
              });
    for (int i = 0; i < 3; i++) {
        if (!componentsToBeUpdated.empty()) {
            rootWidget->printTree();
        }
        for (auto &c: componentsToBeUpdated) {
            c->update();
        }

    }

    rootWidget->printTree();
}

SharedWidget HermesEngine::findSharedWidget(StateWrapperRef &widgetVariable) {
    return widgetVariable->getValue().asObject(*runtime).asHostObject<WidgetHostWrapper>(*runtime)->getNativeWidget();
}

void HermesEngine::componentEffectImpl(const Value &value) {
}

void HermesEngine::prepareForReconcile() {
}

void HermesEngine::pushExistingComponent(const std::shared_ptr<ComponentContext> context) {
    componentContextFactory.emplace(context);
}

void HermesEngine::listConciliar(const shared_ptr<WidgetHostWrapper> &widgetWrapper, Value arr, Value func) {
    auto widget = widgetWrapper->getNativeWidget();
    if (arr.asObject(*runtime).hasProperty(*runtime, "_isStateVariable")) {
        arr = arr.asObject(*runtime).getProperty(*runtime, "value");
    }
    //It's okay if the component still there, but this is very important for the cases where we reconcile without recreating the component
    contextStack.emplace(widget->component());
    auto functionWrapper = StateWrapper::create(*runtime, std::move(func));
    widget->component()->reconcileList(widget, std::make_unique<HermesArray>(*runtime, std::move(arr)),
                                       std::move(functionWrapper));

    contextStack.pop();
}

void HermesEngine::installFunctions() {
    auto &rt = *runtime;

    DEFINE_GLOBAL_FUNCTION("render", 0,
                           [this](Runtime &rt, const Value &thisVal, const Value *args,size_t count) -> Value {

                           if (count == 0) {
                           throw JSINativeException("render requires at least one argument");
                           }
                           render(args[0]);
                           return Value::undefined();
                           });

    DEFINE_GLOBAL_FUNCTION("shutdown", 2,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           shutdown();
                           return Value::undefined();
                           });

    runtime->global().setProperty(rt, "createElement", Function::createFromHostFunction(
                                      rt, PropNameID::forAscii(rt, "createElement"), 2,
                                      [this](Runtime &rt, const Value &thisVal, const Value *args,
                                             size_t count) -> Value {
                                          if (count < 2 || !args[0].isString() || !args[1].isObject()) {
                                              throw JSError(rt, "Invalid arguments for createElement");
                                          }
                                          std::string type = args[0].asString(rt).utf8(rt);
                                          auto &props = args[1];
                                          auto widget = createComponent(type, props);
                                          auto wrapper = std::make_shared<WidgetHostWrapper>(this, widget);
                                          Object obj = Object::createFromHostObject(rt, wrapper);
                                          obj.setExternalMemoryPressure(rt, 5 * 1024 * 1024);
                                          return obj;
                                      }));

    DEFINE_GLOBAL_FUNCTION("useState", 1,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           return useStateImpl(args[0]);
                           });

    DEFINE_GLOBAL_FUNCTION("effect", 0,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           componentEffectImpl(Value(rt,args[0]),args[1]);
                           return Value::undefined();
                           });

    DEFINE_GLOBAL_FUNCTION("beginComponentInit", 0,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           beginComponentImpl();
                           return Value::undefined();
                           });

    DEFINE_GLOBAL_FUNCTION("endComponent", 0,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           endComponentImpl();
                           return Value::undefined();
                           });

    DEFINE_GLOBAL_FUNCTION("listConciliar", 0,
                           [this](Runtime &rt, const Value &thisVal, const Value *args, size_t count) -> Value {
                           auto container=args[0].asObject(rt).asHostObject<WidgetHostWrapper>(rt);
                           auto arrayObject=Value(rt,args[1]);
                           auto function=Value(rt,args[2]);
                           listConciliar(container,std::move(arrayObject),std::move(function));
                           return Value::undefined();
                           });
}

void HermesEngine::execute(const std::shared_ptr<Buffer> buf) const {
    runtime->evaluateJavaScript(buf, "f.js");
}


void HermesEngine::shutdown() {
    _started = false;
    componentsToBeUpdated.clear();
    rootWidget->resetPointer();
}

std::unique_ptr<WidgetHolder> HermesEngine::getWidgetHolder(StateWrapperRef &widgetVariable) {
    auto val = widgetVariable->getValue();
    return getWidgetHolder(std::move(val));
}

std::unique_ptr<WidgetHolder> HermesEngine::getWidgetHolder(Value value) {
    auto &rt = *runtime;
    Object obj = value.asObject(rt);
    const auto isInternal = obj.getProperty(rt, "$$internalComponent").asBool();
    std::unique_ptr<WidgetHolder> holder;
    auto props = obj.getProperty(rt, "props");
    Key key;
    if (obj.hasProperty(rt, "key")) {
        auto keyValue = obj.getProperty(rt, "key");
        if (keyValue.isString()) {
            key = std::move(keyValue.asString(rt).utf8(rt));
        } else if (keyValue.isNumber()) {
            key = std::to_string(static_cast<int>(keyValue.asNumber()));
        }
    }
    if (isInternal) {
        auto componentName = obj.getProperty(rt, "component").asString(rt).utf8(rt);

        std::optional<std::string> id;
        if (obj.hasProperty(rt, "id")) {
            id = obj.getProperty(rt, "id").asString(rt).utf8(rt);
        }
        holder = std::make_unique<HermesWidgetHolder>(rt, componentName, std::make_unique<Value>(rt, props), id,
                                                      std::move(key));
    } else {
        auto func = obj.getProperty(rt, "component");
        holder = std::make_unique<HermesWidgetHolder>(rt, std::make_unique<Value>(rt, func),
                                                      std::make_unique<Value>(rt, props), std::move(key));
    }
    return holder;
}

HermesEngine::~HermesEngine() {
    while (!contextStack.empty()) contextStack.pop();
    pool.finished = true;
    rootWidget.reset();

    componentsToBeUpdated.clear();
    runtime.reset();
}
