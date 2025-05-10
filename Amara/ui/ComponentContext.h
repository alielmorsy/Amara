#ifndef COMPONENTCONTEXT_H
#define COMPONENTCONTEXT_H
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "../runtime/hermes/StateWrapper.h"
#include "../runtime/WidgetHolder.h"
#include "../runtime/AmaraArray.h"
class ContainerWidget;
class Widget;
using EffectCleanup = std::optional<std::function<void()> >;
using EffectCallback = std::function<EffectCleanup()>;
using SetStateFunction = std::function<void(StateWrapperRef)>;
using EmptyFunction = std::function<void()>;
class IEngine;
class Widget;

static size_t currentIndex = 0;

class ComponentContext {
private:
    bool _reconciliationStarted = false;
    bool insideReconciliation = false;
    IEngine *engine;
    size_t hookCount = 0;
    bool updating;

    struct Effect {
        StateWrapperRef callback;
        std::vector<StateWrapperRef> deps;

        StateWrapperRef cleanup;

        Effect(StateWrapperRef callback, std::vector<StateWrapperRef> deps,
               StateWrapperRef cleanup)
            : callback(std::move(callback)),
              deps(std::move(deps)),
              cleanup(std::move(cleanup)) {
        }
    };

    struct State {
        StateWrapperRef object;
        std::vector<std::unique_ptr<StateWrapper> > pendingUpdates;
    };

    void _updateStates();

    bool dirty = false;
    //widgets inside the component

    std::vector<Effect> effects;
    std::vector<State> states;
    std::unordered_map<size_t, StateWrapperRef> toBeUpdated;
    std::unordered_map<size_t, StateWrapperRef> updatedStates;
    size_t _index;

public:
    explicit ComponentContext(IEngine *engine): engine(engine), _index(currentIndex++) {
    }

    std::vector<std::shared_ptr<Widget> > widgets;

    std::tuple<StateWrapper *, SetStateFunction> useState(StateWrapperRef value, EmptyFunction notifier);

    void effect(StateWrapperRef fn, std::vector<StateWrapperRef> deps);

    void reconcileChildren(std::shared_ptr<ContainerWidget> &shared,
                           std::shared_ptr<ContainerWidget> &container_widget);

    std::shared_ptr<Widget> reconcileObject(const std::shared_ptr<Widget> &old,
                                            std::unique_ptr<WidgetHolder> &newCaller);

    void update();

    bool reconciliationStarted() const {
        return _reconciliationStarted;
    }

    void reconcileList(const std::shared_ptr<Widget> &listHolder, std::unique_ptr<AmaraArray> arr,
                       std::unique_ptr<StateWrapper> func);

    void reconcileWidgetHolders(const std::shared_ptr<ContainerWidget> &listHolder,
                                std::vector<std::unique_ptr<WidgetHolder>> widgetHolders);

    void markDirty() {
        dirty = true;
    };

    std::weak_ptr<ContainerWidget> reconcilingObject;

    ~ComponentContext() {
        widgets.clear();
        int x = 0;
    }

    size_t index() {
        return _index;
    }
};


#endif
