#ifndef HERHMES_INVOKER_H
#define HERHMES_INVOKER_H
#include <hermes/hermes.h>
#include <jsi/jsi.h>
#include "../Invoker.h"
using namespace facebook::jsi;

class HermesInvoker : public Invoker {
public:
    HermesInvoker(Runtime &runtime, std::shared_ptr<Value> value): runtime(runtime), value(value),
                                                                   func(value->asObject(runtime).asFunction(runtime)) {
    }

    void invoke(std::initializer_list<StateWrapperRef> wrapper) override {
    }

    Runtime &runtime;
    std::shared_ptr<Value> value;
    Function func;
};

#endif //HERHMES_INVOKER_H
