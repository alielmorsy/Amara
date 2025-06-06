//
// Created by Ali Elmorsy on 5/22/2025.
//

#ifndef VALUEWRAPPER_H
#define VALUEWRAPPER_H

#include "ObjectMap.h"

namespace amara {
    enum ValueType {
        UNDEFINED,

        INT,
        DOUBLE,
        STRING,
        OBJECT,
        ARRAY,
    };

    class ValueWrapper {
    public:
        virtual ~ValueWrapper() = default;

        virtual std::unique_ptr<ObjectMap> asPropMap() =0;

        virtual std::unique_ptr<ValueWrapper> callAsFunction(std::vector<std::unique_ptr<ValueWrapper> > args) =0;

        virtual std::shared_ptr<void> getRawHostObject() = 0;

        template<typename T>
        std::shared_ptr<T> asHostObject() {
            return std::static_pointer_cast<T>(getRawHostObject());
        }
    protected:
        ValueType _type = ValueType::UNDEFINED;
    };
}
#endif //VALUEWRAPPER_H
