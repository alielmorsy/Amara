//
// Created by Ali Elmorsy on 4/5/2025.
//

#ifndef HERMESWIDGETOLDER_H
#define HERMESWIDGETOLDER_H

#include <memory>

#include "HermesPropMap.h"
#include "Invoker.h"


#include "../WidgetHolder.h"

class Widget;

class HermesWidgetHolder : public WidgetHolder {
public:
    ~HermesWidgetHolder() override = default;

    HermesWidgetHolder(Runtime &rt, std::unique_ptr<Value> componentFunction, std::unique_ptr<HermesPropMap> props,
                       const Key &key = Key())
        : WidgetHolder(key, std::move(props)), componentFunction(std::move(componentFunction)),
          rt(rt) {
    }

    HermesWidgetHolder(Runtime &rt, std::string componentName, std::unique_ptr<HermesPropMap> props,
                       std::optional<std::string> id = std::nullopt,
                       Key key = Key()) : WidgetHolder(std::move(componentName), std::move(props), std::move(id), key),
                                          rt(rt) {
    }

    std::shared_ptr<Widget> execute(IEngine *engine) override;

    std::vector<std::unique_ptr<WidgetHolder> > getChildren() override;

    static std::unique_ptr<HermesWidgetHolder> create(Runtime &rt, const Value &value) {
        Object obj = value.asObject(rt);
        const auto isInternal = obj.getProperty(rt, "$$internalComponent").asBool();
        std::unique_ptr<HermesWidgetHolder> holder;
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
        auto propMap = std::make_unique<HermesPropMap>(rt, std::move(props));
        if (isInternal) {
            auto componentName = obj.getProperty(rt, "component").asString(rt).utf8(rt);

            std::optional<std::string> id;
            if (obj.hasProperty(rt, "id")) {
                id = obj.getProperty(rt, "id").asString(rt).utf8(rt);
            }
            holder = std::make_unique<HermesWidgetHolder>(rt, componentName, std::move(propMap), id,
                                                          std::move(key));
        } else {
            auto func = obj.getProperty(rt, "component");

            holder = std::make_unique<HermesWidgetHolder>(rt, std::make_unique<Value>(rt, func), std::move(propMap),
                                                          std::move(key));
        }
        return holder;
    }

    bool sameComponent(StateWrapperRef &other) override;

private:
    std::unique_ptr<Value> componentFunction;
    Runtime &rt;
};


#endif //HERMESWIDGETOLDER_H
