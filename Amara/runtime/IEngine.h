#ifndef IENGINE_H
#define IENGINE_H
#include <complex.h>
#include <stack>

#include "../utils/WidgetPool.h"
#include "hermes/HermesPropMap.h"

class IEngine {
public:
    virtual ~IEngine() = default;

    virtual void beginComponentImpl() =0;

    virtual void endComponentImpl() =0;

    virtual std::shared_ptr<Widget> createComponent(std::string &type, std::unique_ptr<PropMap> propsMap) =0;

    virtual void installFunctions() =0;

    virtual void shutdown() =0;

    virtual void prepareForReconcile() =0;

    virtual void pushExistingComponent(std::shared_ptr<ComponentContext> context) =0;

    virtual SharedWidget findSharedWidget(StateWrapperRef &widgetVariable) =0;

    virtual std::unique_ptr<WidgetHolder> getWidgetHolder(StateWrapperRef &widgetVariable) =0;

    void plugComponent(const std::shared_ptr<ComponentContext> &component) {
        contextStack.emplace(component);
    }

    virtual void unplugComponent() {
        contextStack.pop();
    };

    virtual void compareProps(const std::unique_ptr<PropMap> &old, const std::unique_ptr<PropMap> &newMap) =0;

protected:
    SharedWidget rootWidget;
    WidgetPool pool;
    std::stack<std::shared_ptr<ComponentContext> > contextStack;
    std::stack<std::shared_ptr<ComponentContext> > componentContextFactory;
};
#endif
