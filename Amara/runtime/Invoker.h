#ifndef INVOKER_H
#define INVOKER_H
#include <any>
#include "hermes/StateWrapper.h"

class Invoker {
public:

    virtual void invoke(std::initializer_list<StateWrapperRef>  wrapper) =0;
};


#endif
