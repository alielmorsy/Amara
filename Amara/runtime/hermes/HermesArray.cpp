//
// Created by Ali Elmorsy on 4/9/2025.
//

#include "HermesArray.h"

size_t HermesArray::size() {
    return arr.size(rt);
}

StateWrapperRef HermesArray::getValue(size_t index) {
    return StateWrapper::create(rt, arr.getValueAtIndex(rt, index));
}
