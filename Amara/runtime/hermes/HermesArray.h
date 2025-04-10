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
    HermesArray(Runtime &rt, Value arr) : arr(arr.asObject(rt).asArray(rt)), rt(rt) {
    }

    size_t size() override;

    StateWrapperRef getValue(size_t index) override;

    ~HermesArray() override = default;

private:
    Array arr;
    Runtime &rt;
};


#endif //HERMESARRAY_H
