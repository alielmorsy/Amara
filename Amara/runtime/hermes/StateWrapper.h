#ifndef STATEWRAPPER_H
#define STATEWRAPPER_H
#include <hermes/hermes.h>
using namespace facebook::jsi;

class StateWrapper;
typedef std::unique_ptr<StateWrapper> StateWrapperRef;


class StateWrapper {
private:
    Runtime &rt;
    Value value;

public:
    explicit StateWrapper(Runtime &rt, Value value) : rt(rt),
                                                      value(std::move(value)) {
    };

    static StateWrapperRef create(Runtime &rt, Value value) {
        auto wrapper = std::make_unique<StateWrapper>(rt, std::move(value));
        return wrapper;
    }

    void setValue(const StateWrapperRef &newValue) const {
        this->value.asObject(rt).setProperty(rt, "value", newValue->value);
    }

    [[nodiscard]] StateWrapperRef getInternalValue() const {
        auto value = this->value.asObject(rt).getProperty(rt, "value");
        return create(rt, std::move(value));
    }

    [[nodiscard]] Value getValue() const {
        return Value(rt, value);
    }

    bool equals(StateWrapper *other) const {
        return Value::strictEquals(rt, value, other->value);
    }

    template<typename... Args>
    StateWrapperRef call(Args... args) {
        auto result = value.asObject(rt).asFunction(rt).call(rt, std::forward<Args>(args)...);
        return create(rt, std::move(result));
    }

    [[nodiscard]] bool hasValue() const {
        return !value.isUndefined();
    }



};
#endif //WRAPPER_H
