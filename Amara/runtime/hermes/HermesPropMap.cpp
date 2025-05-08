#include "HermesPropMap.h"

#include "../../utils/ScopedTimer.h"

#define FALLBACK_IF if (!has(key)) return std::move(defaultValue)

std::string HermesPropMap::getString(const std::string &key, const std::string &defaultValue) const {
    auto val = get(key);
    if (val.isNumber()) {
        return std::to_string(value->asNumber());
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

std::unique_ptr<PropMap> HermesPropMap::getObject(const std::string &key) const {
    auto value = get(key);
    if (!value.isObject()) {
        return nullptr;
    }
    return std::make_unique<HermesPropMap>(runtime, std::make_shared<Value>(runtime, value));
}

Value HermesPropMap::get(const std::string &key) const {
    return obj.getProperty(runtime, key.c_str());
}
