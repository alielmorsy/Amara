//
// Created by Ali Elmorsy on 4/5/2025.
//

#ifndef HERMESWIDGETOLDER_H
#define HERMESWIDGETOLDER_H

#include <memory>

#include "Invoker.h"


#include "../WidgetHolder.h"

class Widget;

class HermesWidgetHolder : public WidgetHolder {
public:
    ~HermesWidgetHolder() override = default;

    HermesWidgetHolder(Runtime &rt, std::unique_ptr<Value> componentFunction, std::unique_ptr<Value> props,
                       const Key &key = Key())
        : WidgetHolder(key), componentFunction(std::move(componentFunction)), props(std::move(props)),
          rt(rt) {
    }

    HermesWidgetHolder(Runtime &rt, std::string componentName, std::unique_ptr<Value> props,
                       std::optional<std::string> id = std::nullopt,
                       Key key = Key()) : WidgetHolder(std::move(componentName),std::move(id), key),
                                          props(std::move(props)), rt(rt) {
    }

    std::shared_ptr<Widget> execute(IEngine *engine) override;

private:
    std::unique_ptr<Value> componentFunction;
    std::unique_ptr<Value> props;
    Runtime &rt;
};


#endif //HERMESWIDGETOLDER_H
