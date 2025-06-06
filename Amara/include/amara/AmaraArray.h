//
// Created by Ali Elmorsy on 4/9/2025.
//

#ifndef AMARAARRAY_H
#define AMARAARRAY_H
#include "ValueWrapper.h"
namespace amara{

    class AmaraArray {
    public:
        virtual ~AmaraArray() = default;

        AmaraArray() = default;

        //No copying
        AmaraArray(const AmaraArray &) = delete;

        virtual size_t size() =0;

        virtual ValueWrapper getValue(size_t index) =0;
    };
}
#endif //AMARAARRAY_H
