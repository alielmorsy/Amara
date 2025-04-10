#ifndef KEY_H
#define KEY_H

struct Key {
    std::string key;

    [[nodiscard]] bool hasKey() const {
        return !key.empty();
    }

    constexpr Key() = default;


    explicit Key(std::string value) : key(std::move(value)) {
    }

    Key &operator=(const Key &other) {
        if (this != &other) {
            // Avoid self-assignment
            key = other.key; // Copy the key from another Key object
        }
        return *this; // Return the current object to allow chain assignment
    }

    Key &operator=(std::string key) {
        this->key = key;
        return *this; // Return the current object to allow chain assignment
    }

    // Equality operator (checks if keys are equal)
    bool operator==(const Key &other) const {
        return key == other.key; // Compare the keys (strings)
    }
};
#endif //KEY_H
