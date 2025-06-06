#ifndef WIDGETHOLDER_H
#define WIDGETHOLDER_H

#include <utility>
#include <optional>
#include <memory>
#include <string>
#include "Key.h"
#include "PropMap.h"
#include <vector>

namespace amara {
    class Widget;
    class IEngine;

    class WidgetHolder {
    public:
        explicit WidgetHolder(const Key &&key, std::unique_ptr<PropMap> props)
            : _props(std::move(props)), _key(key) {
        }

        WidgetHolder(std::optional<std::string> componentName,
                     std::unique_ptr<PropMap> props,
                     std::optional<std::string> id = std::nullopt,
                     const Key &key = Key())
            : componentName(std::move(componentName)),
              _props(std::move(props)),
              id(std::move(id)),
              _key(key),
              isInternal(true) {
        }

        virtual ~WidgetHolder() = default;

        // Delete copy
        WidgetHolder(const WidgetHolder &) = delete;

        WidgetHolder &operator=(const WidgetHolder &) = delete;

        // Allow move
        WidgetHolder(WidgetHolder &&) = default;

        WidgetHolder &operator=(WidgetHolder &&) = default;

        virtual std::shared_ptr<Widget> execute(IEngine *) = 0;

        bool isComponent() const {
            return !isInternal;
        }

        [[nodiscard]] bool hasID() const {
            return id.has_value();
        }

        [[nodiscard]] std::string_view getID() const {
            return id.value();
        }

        [[nodiscard]] const Key &key() const {
            return _key;
        }

        [[nodiscard]] std::string getComponentName() const {
            return componentName.value();
        }

        virtual std::vector<std::unique_ptr<WidgetHolder> > getChildren() = 0;

        virtual std::vector<std::string> getTextChildren() = 0;

        const PropMap &props() const {
            return *_props;
        }

        virtual bool sameComponent(StateWrapperRef &other) = 0;

    protected:
        bool isInternal = false;
        std::optional<std::string> id;
        std::optional<std::string> componentName;
        std::unique_ptr<PropMap> _props;
        Key _key;
    };
}
#endif //WIDGETHOLDER_H
