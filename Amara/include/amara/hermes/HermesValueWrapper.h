//
// Created by Ali Elmorsy on 5/24/2025.
//

#ifndef HERMESVALUEWRAPPER_H
#define HERMESVALUEWRAPPER_H
#include "amara/ValueWrapper.h"

namespace amara {
    class HermesValueWrapper : public ValueWrapper {
    public:
        HermesValueWrapper(Runtime &runtime, Value &&value): _runtime(runtime), _value(std::move(value)) {
        }

        std::unique_ptr<ValueWrapper> callAsFunction(std::vector<std::unique_ptr<ValueWrapper> > args) override {
            assert(_value.isObject(), "Trying to call a function that is not an object ");
            auto obj = _value.asObject(_runtime);
            auto func = obj.asFunction(_runtime);
            std::vector<const Value> hermesArgs; // To store extracted Values

            for (const auto &arg: args) {
                // It's a safe downcast because you cannot use both runtimes at the same time
                auto *hermesArg = static_cast<HermesValueWrapper *>(arg.get());

                hermesArgs.push_back(hermesArg->_value);
            }

            // Now you can call the Hermes function
            auto result = func.call(_runtime, hermesArgs.data(), hermesArgs.size());
            return std::make_unique<HermesValueWrapper>(_runtime, std::move(result));
        }

        std::shared_ptr<void> getRawHostObject() override {
            return _value.asObject(_runtime).asHostObject(_runtime);
        };

    private:
        Runtime &_runtime;
        Value _value;
    };
}
#endif //HERMESVALUEWRAPPER_H
