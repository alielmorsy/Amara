//
// Created by Ali Elmorsy on 4/9/2025.
//

#ifndef HERMESARRAY_H
#define HERMESARRAY_H

#include "../AmaraArray.h"
#include "jsi/jsi.h"
using namespace facebook::jsi;

class HermesArray : public AmaraArray {
public:
    HermesArray(Runtime &rt, Value arr)
        : arr(resolveArray(rt, std::move(arr))), rt(rt) {
    }

    size_t size() override;

    StateWrapperRef getValue(size_t index) override;

    ~HermesArray() override = default;

private:
    static Array resolveArray(Runtime &rt, Value val) {
        auto obj = val.asObject(rt);
        if (obj.isFunction(rt)) {
            return obj.asFunction(rt).call(rt).asObject(rt).asArray(rt);
        } else {
            auto arr = obj.asArray(rt);
            return arr;
        }
    }

    Array arr;
    Runtime &rt;
};


#endif //HERMESARRAY_H
