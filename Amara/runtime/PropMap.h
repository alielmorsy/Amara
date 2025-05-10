#ifndef PROPMAP_H
#define PROPMAP_H
#include <string>
#include <memory>

#include "AmaraArray.h"

class PropMap {
public:
    virtual ~PropMap() = default;

    virtual double getNumber(const std::string &key, double defaultValue = 0) const = 0;

    virtual std::string getString(const std::string &key, const std::string &defaultValue = "") const = 0;

    virtual bool getBool(const std::string &key, bool defaultValue = false) const = 0;

    virtual std::unique_ptr<void, void(*)(void *)> getFunction(const std::string &key) const = 0;

    virtual std::unique_ptr<PropMap> getObject(const std::string &key) const = 0;

    virtual std::unique_ptr<AmaraArray> getArray(const std::string &key) const = 0;

    virtual bool has(const std::string &key) const = 0;

    virtual void remove(const std::string &key) const = 0;

    virtual void set(const std::string &key, double value) const = 0;

    virtual void set(const std::string &key, std::unique_ptr<PropMap> &map) const = 0;

    virtual void set(const std::string &key, const std::string &value) const = 0;
};
#endif //PROPMAP_H
