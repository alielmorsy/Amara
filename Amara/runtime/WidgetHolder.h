#ifndef WIDGETHOLDER_H
#define WIDGETHOLDER_H

#include <utility>

#include "PropMap.h"
#include "../ui/Key.h"
class Widget;
class IEngine;

class WidgetHolder {
public:
    explicit WidgetHolder(const Key &key,
                          std::unique_ptr<PropMap> props) : _key(key), _props(std::move(props)) {
    };

    WidgetHolder(std::optional<std::string> componentName, std::unique_ptr<PropMap> props,
                 std::optional<std::string> id,
                 const Key &key = Key()): componentName(std::move(componentName)), _props(std::move(props)),
                                          id(std::move(id)), _key(key) {
        isInternal = true;
    }

    virtual ~WidgetHolder() = default;

    virtual std::shared_ptr<Widget> execute(IEngine *) =0;

    bool isComponent() {
        return !isInternal;
    }

    bool hasID() {
        return id.has_value();
    }

    std::string &getID() {
        return id.value();
    }

    Key &key() {
        return _key;
    }

    [[nodiscard]] std::string getComponentName() const {
        return componentName.value();
    }

    virtual std::vector<std::unique_ptr<WidgetHolder> > getChildren() =0;

    virtual std::vector<std::string> getTextChildren() =0;

    std::unique_ptr<PropMap> &props() {
        return _props;
    }

    virtual bool sameComponent(StateWrapperRef &other) =0;

protected:
    bool isInternal = false;
    std::optional<std::string> id;
    //Usable only for internal widgets
    std::optional<std::string> componentName;
    std::unique_ptr<PropMap> _props;
    Key _key;
};
#endif //WIDGETHOLDER_H
