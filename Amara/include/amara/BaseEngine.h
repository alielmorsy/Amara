#ifndef IENGINE_H
#define IENGINE_H
#include <stack>
#include <string>
#include "WidgetHolder.h"
#include <amara/ValueWrapper.h>
#include "ObjectMap.h"
#include "ComponentContext.h"
#include "amara/widgets/Widget.h"

namespace amara {
    class BaseEngine {
    public:
        void beginComponent(std::string id) {
            auto context = new ComponentContext(this, std::move(id));
            //Assigning the parent context (if any) to the _children
            if (!_componentStack.empty()) {
                auto parent = _componentStack.top();
                parent->_children.push_back(context);
            }
            _componentStack.push(context);
        }


        virtual ~BaseEngine() = default;


        virtual void execute(std::string path) =0;

        virtual std::unique_ptr<ValueWrapper> callFunction(std::unique_ptr<ValueWrapper>,
                                                           std::vector<std::unique_ptr<ValueWrapper> > args) =0;

    protected:
        std::stack<ComponentContext *> _componentStack;

        virtual void initialize() =0;


        virtual void endComponent() =0;


        std::unique_ptr<WidgetHolder> convertToWidgetHolder(ValueWrapper &&wrapper);

        virtual widgets::Widget* createWidget(std::string &type, std::unique_ptr<ObjectMap> propsMap) override; =0;

        virtual void prepareRender(std::unique_ptr<ValueWrapper> wrapper) =0;

        inline void returnToPool(widgets::Widget *value);

        void unplugComponent() {
            _componentStack.pop();
        };

    private:
        friend ComponentContext;
    };
}
#endif //IENGINE_H
