#ifndef _SERIALIZE_H_
#define _SERIALIZE_H_
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

class Compressed {
public:
    std::vector<uint8_t> compressed_data;
    int64_t cur;

    Compressed(std::vector<uint8_t>& compressed_data_) : compressed_data(compressed_data_) {
        cur = 0;
    }
    bool end() {
        return (cur >= compressed_data.size()) ? true : false;
    }
    uint8_t* getNowHeader() {
        if (cur >= compressed_data.size()) {
            throw std::runtime_error("Not enough data");
        }
        return compressed_data.data() + cur;
    }

    void moveCur(int offset) {
        cur += offset;
    }
};

std::vector<uint8_t> serialize(const std::string& input);

std::vector<uint8_t> serialize(const int input);

std::vector<uint8_t> serialize(const char input);

std::vector<uint8_t> serialize(const uint16_t input);

// std::string deserialize_string(Compressed& compressed);

int deserialize_int(Compressed& compressed);

uint8_t deserialize_uint8_t(Compressed& compressed);

uint16_t deserialize_uint16_t(Compressed& compressed);

#endif