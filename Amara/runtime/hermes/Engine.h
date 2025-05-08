//
// Created by Ali Elmorsy on 3/31/2025.
//

#ifndef HERMES_ENGINE_H
#define HERMES_ENGINE_H


#include <memory>

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include "WidgetHostWrapper.h"
#include "../../ui/ComponentContext.h"
#include "../IEngine.h"
#include "../../ui/Widget.h"

#define DEFINE_GLOBAL_FUNCTION(name,paramCount,func) runtime->global().setProperty(rt, name, Function::createFromHostFunction( \
                                      rt, PropNameID::forAscii(rt, name),paramCount,func))
using namespace facebook::jsi;

class HermesEngine : public IEngine {
public:
    explicit HermesEngine(std::unique_ptr<Runtime> runtime): runtime(std::move(runtime)) {
    }

    ~HermesEngine() override;

    std::shared_ptr<Widget> createComponent(std::string &type, const Value &props) override;

    void installFunctions() override;

    void execute(std::shared_ptr<Buffer> buf) const;

    void componentEffectImpl(Value fn, const Value &deps);

    void prepareForReconcile() override;

    void pushExistingComponent(std::shared_ptr<ComponentContext> context) override;

    void listConciliar(const shared_ptr<WidgetHostWrapper> &widgetWrapper, Value arr, Value func);

private:
    void beginComponentImpl() override;

    void endComponentImpl() override;

    Value useStateImpl(const Value &value);

    void componentEffectImpl(const Value &value);

    void render(const Value &value);

public:
    SharedWidget findSharedWidget(StateWrapperRef &widgetVariable) override;

    void shutdown() override;

    std::unique_ptr<WidgetHolder> getWidgetHolder(StateWrapperRef &widgetVariable) override;
    std::unique_ptr<WidgetHolder> getWidgetHolder(const Value &value);


private:
    bool _started = false;
    std::unique_ptr<Runtime> runtime;

    std::vector<std::shared_ptr<ComponentContext> > componentsToBeUpdated;
    std::vector<std::shared_ptr<ComponentContext> > nextIterationComponents;
    std::shared_ptr<WidgetHostWrapper> randomWrapper;
};


#endif //HERMES_ENGINE_H
