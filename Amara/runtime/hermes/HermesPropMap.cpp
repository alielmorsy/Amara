#include "HermesPropMap.h"

#include "HermesArray.h"
#include "../../utils/ScopedTimer.h"

#define FALLBACK_IF(val) if (val.isUndefined()) return std::move(defaultValue);

std::string HermesPropMap::getString(const std::string &key, const std::string &defaultValue) const {
    auto val = get(key);
    FALLBACK_IF(val)
    if (val.isNumber()) {
        return std::to_string(val.asNumber());
    }
    return val.asString(runtime).utf8(runtime);
}

bool HermesPropMap::getBool(const std::string &key, bool defaultValue) const {
    return get(key).asBool();
}

std::unique_ptr<void, void(*)(void *)> HermesPropMap::getFunction(const std::string &key) const {
    auto val = get(key);
    if (!val.isObject()) {
        return {
            nullptr, [](void *) {
            }
        }; // Empty deleter for nullptr
    }

    // Custom deleter to destroy HermesInvoker
    auto deleter = [](void *ptr) {
        delete static_cast<HermesInvoker *>(ptr);
    };


    return {new HermesInvoker(runtime, std::make_shared<Value>(runtime, val)), std::move(deleter)};
}

double HermesPropMap::getNumber(const std::string &key, double defaultValue) const {
    // FALLBACK_IF;
    auto value = get(key);
    if (value.isString()) {
        try {
            return std::stoi(value.asString(runtime).utf8(runtime));
        } catch (const std::invalid_argument &) {
            //TODO
            //logError("Failed to parse string as integer for prop: " + key);
            return defaultValue;
        }
    }
    return value.asNumber();
}

bool HermesPropMap::has(const std::string &key) const {
    //ScopedTimer timer;
    return obj.hasProperty(runtime, key.c_str());
}

void HermesPropMap::remove(const std::string &key) const {
    obj.setProperty(runtime, key.c_str(), Value());
}

std::unique_ptr<PropMap> HermesPropMap::getObject(const std::string &key) const {
    auto value = get(key);
    if (!value.isObject()) {
        return nullptr;
    }
    return std::make_unique<HermesPropMap>(runtime, std::move(value));
}

std::unique_ptr<AmaraArray> HermesPropMap::getArray(const std::string &key) const {
    auto value = get(key);
    if (value.isUndefined()) return  nullptr;
    auto arr = value.asObject(runtime).asArray(runtime);
    return std::make_unique<HermesArray>(runtime, std::move(arr));
}

Value HermesPropMap::get(const std::string &key) const {
    return obj.getProperty(runtime, key.c_str());
}

void HermesPropMap::set(const std::string &key, double value) const {
    obj.setProperty(runtime, key.c_str(), Value(value));
}

void HermesPropMap::set(const std::string &key, std::unique_ptr<PropMap> &map) const {
    obj.setProperty(runtime, key.c_str(), dynamic_cast<HermesPropMap *>(map.get())->obj);
}

void HermesPropMap::set(const std::string &key, const std::string &value) const {
    obj.setProperty(runtime, key.c_str(), Value(String::createFromAscii(runtime, value.c_str())));
}
