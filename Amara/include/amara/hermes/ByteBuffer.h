//
// Created by Ali Elmorsy on 5/24/2025.
//

#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H


#include <jsi/jsi.h>

class ByteBuffer : public facebook::jsi::Buffer {
public:
    ByteBuffer(uint8_t *data, const size_t size): _data(data), _size(size) {
    }

    ~ByteBuffer() override {
        delete[] _data;
    };

    [[nodiscard]] size_t size() const override {
        return _size;
    };

    [[nodiscard]] const uint8_t *data() const override {
        return _data;
    };

private:
    uint8_t *_data;
    size_t _size;
};


#endif //BYTEBUFFER_H
