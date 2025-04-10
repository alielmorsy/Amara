#ifndef ENGINE_H
#define ENGINE_H
#include <complex.h>
#include <unordered_map>

#include "jsi/jsi.h"

#include "Element.h"
#include "Widget.h"
using namespace facebook::jsi;

class Engine {
private:
    std::vector<int> updatedIndices;
    std::unique_ptr<Widget> rootWidget;
    int counter = 0;
    std::unordered_map<int, std::shared_ptr<BaseElement> > previousElements;
    bool renderRquested = false;

    struct ComponentState {
        std::vector<std::shared_ptr<Value> > states; // Stores state values for each useState call
        size_t hookIndex = 0; // Tracks the current hook call during rendering
    };

    std::unordered_map<int, ComponentState> componentStates; // Maps component index to its state
    int currentComponentIndex = -1;

public:
    std::unordered_map<int, std::shared_ptr<BaseElement> > elements;

    Engine() {
        elements.reserve(10);
    }

    ~Engine();

    int createElementTree(Runtime &rt, const Value &type, const Value &props, const Value &key);

    void render(Runtime &rt, int index, bool initial=true);

    void partialUpdate(Runtime &rt);

    std::unique_ptr<Widget> buildWidget(Runtime &rt, int index, std::unique_ptr<Widget> prevWidget);

    facebook::jsi::Value useStateImpl(facebook::jsi::Runtime &rt, const facebook::jsi::Value &thisValue,
                                      const facebook::jsi::Value *args, size_t count);

    void buidWidget(Runtime &rt, int index);
};


#endif //ENGINE_H
