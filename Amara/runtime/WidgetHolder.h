#ifndef WIDGETHOLDER_H
#define WIDGETHOLDER_H

#include <utility>
#include "../ui/Key.h"
class Widget;
class IEngine;

class WidgetHolder {
public:
    explicit WidgetHolder(const Key &key = Key()) : _key(key) {
    };

    WidgetHolder(std::optional<std::string> componentName, std::optional<std::string> id,
                 const Key &key = Key()): componentName(std::move(componentName)),
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

protected:
    bool isInternal = false;
    std::optional<std::string> id;
    //Usable only for internal widgets
    std::optional<std::string> componentName;
    Key _key;
};
#endif //WIDGETHOLDER_H
