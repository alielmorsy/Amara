//
// Created by Ali Elmorsy on 4/1/2025.
//

#ifndef HERMESPROPMAP_H
#define HERMESPROPMAP_H
#include "../PropMap.h"
#include <jsi/jsi.h>
#include "Invoker.h"
#include <utility>
using namespace facebook::jsi;

class HermesPropMap : public PropMap {
public:
    HermesPropMap(Runtime &runtime, Value value): obj(value.asObject(runtime)), runtime(runtime) {
    }


    double getNumber(const std::string &key, double defaultValue) const override;

    [[nodiscard]] std::string getString(const std::string &key, const std::string &defaultValue) const override;

    bool getBool(const std::string &key, bool defaultValue) const override;

    std::unique_ptr<void, void(*)(void *)> getFunction(const std::string &key) const override;


    bool has(const std::string &key) const override;

    void remove(const std::string &key) const override;

    std::unique_ptr<PropMap> getObject(const std::string &key) const override;

    std::unique_ptr<AmaraArray> getArray(const std::string &key) const override;

    Value get(const std::string &key) const;

    void set(const std::string &key, double value) const;

    void set(const std::string &key, std::unique_ptr<PropMap> &map) const;

    void set(const std::string &key, const std::string &value) const;

    const Object &getHermesValue() const {
        return obj;
    }

private:
    Object obj;
    Runtime &runtime;
};


#endif //HERMESPROPMAP_H
