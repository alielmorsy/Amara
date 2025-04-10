#ifndef ELEMENT_H
#define ELEMENT_H
#include <optional>
#include <variant>
#include <memory>
using namespace facebook::jsi;

enum ElementType {
    PRIMITIVE = 0,
    COMPONENT
};

using namespace std;

struct Key {
    enum class Type { ASCII, INDEX } type;

    union {
        std::string ascii;
        int idx;
    };

    explicit Key(std::string &s) : type(Type::ASCII), ascii(std::move(s)) {
    } // String constructor
    explicit Key(int i) : type(Type::INDEX), idx(i) {
    } // Integer constructor

    ~Key() {
        if (type == Type::ASCII) {
            ascii.~basic_string();
        }
    }

    Key(const Key &other) : type(other.type) {
        if (type == Type::ASCII) {
            new(&ascii) std::string(other.ascii); // Copy string safely
        } else {
            idx = other.idx;
        }
    }

    bool operator<(const Key &other) const {
        // Different types: ASCII comes before INDEX
        if (type != other.type) {
            return type < other.type;
        }

        // Same type - compare contents
        switch (type) {
            case Type::ASCII:
                return ascii < other.ascii;
            case Type::INDEX:
                return idx < other.idx;
            default:
                return false; // Should never reach here
        }
    }


    Key &operator=(Key other) noexcept {
        // Copy-and-swap
        swap(*this, other);
        return *this;
    }
    bool operator==(const Key& other) const {
        if (type != other.type) return false;
        switch (type) {
            case Type::ASCII: return ascii == other.ascii;
            case Type::INDEX: return idx == other.idx;
            default: return false;
        }
    }

    friend void swap(Key &a, Key &b) noexcept {
        using std::swap;
        if (a.type == Type::ASCII && b.type == Type::ASCII) swap(a.ascii, b.ascii);
        else if (a.type == Type::INDEX && b.type == Type::INDEX) swap(a.idx, b.idx);
        else {
            Key tmp = std::move(a);
            a = std::move(b);
            b = std::move(tmp);
        }
    }
};
namespace std {
    template<> struct hash<Key> {
        size_t operator()(const Key& k) const noexcept {
            using std::hash;
            size_t hash_value = 0;
            if (k.type == Key::Type::ASCII) {
                hash_value = hash<string>{}(k.ascii);
            } else {
                hash_value = hash<int>{}(k.idx);
            }
            return hash_value ^ (hash<int>{}(static_cast<int>(k.type)) << 1);
        }
    };
}
class BaseElement {
public:
    std::optional<Key> key;

    virtual ~BaseElement() = default;

    template<typename T>
    T *as() {
        return dynamic_cast<T *>(this);
    }

protected:
    ElementType type;

    explicit BaseElement(ElementType type) : type(type) {
    }

public:
    bool isComponent() { return type == COMPONENT; }
};

class PrimitiveElement : public BaseElement {
public:
    std::shared_ptr<Value> props;

    explicit PrimitiveElement(std::string &primitive, std::shared_ptr<Value> &props): BaseElement(PRIMITIVE),
        _primitive(std::move(primitive)), props(props) {
    }

    std::string &primitive() { return _primitive; }

    ~PrimitiveElement() override = default;

private:
    std::string _primitive;
};

class ComponentElement : public BaseElement {
public:
    std::shared_ptr<Value> props;

    explicit ComponentElement(std::shared_ptr<Value> &value, std::shared_ptr<Value> &props) : BaseElement(COMPONENT),
        value(value),
        props(props) {
    }

    [[nodiscard]] std::shared_ptr<Value> componentFunction() const { return value; }

    ~ComponentElement() override {
        value.reset();
        props.reset();
    };

private:
    std::shared_ptr<Value> value;
};
#endif //ELEMENT_H
