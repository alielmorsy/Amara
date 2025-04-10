//
// Created by Ali Elmorsy on 4/9/2025.
//

#ifndef AMARAARRAY_H
#define AMARAARRAY_H
#include "hermes/StateWrapper.h"

class AmaraArray {
public:
    virtual ~AmaraArray() = default;

    AmaraArray() = default;

    //No copying
    AmaraArray(const AmaraArray &) = delete;

    virtual size_t size() =0;

    virtual StateWrapperRef getValue(size_t index) =0;
};
#endif //AMARAARRAY_H
