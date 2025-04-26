#include "serialize.h"
#include <vector>
#include <cstdint>
#include <cstring>

int int_tot = 0;
int string_tot = 0;
int char_tot = 0;

std::vector<uint8_t> serialize(const std::string& input) {
    string_tot ++;
    std::vector<uint8_t> serialized_data;
    uint32_t size = input.size(); 
    serialized_data.insert(serialized_data.end(), reinterpret_cast<uint8_t*>(&size), reinterpret_cast<uint8_t*>(&size) + sizeof(size));
    serialized_data.insert(serialized_data.end(), input.begin(), input.end());
    return serialized_data;
}


std::vector<uint8_t> serialize(const int input) {
    int_tot ++;
    std::vector<uint8_t> serialized_data(sizeof(input));
    std::memcpy(serialized_data.data(), &input, sizeof(input));
    return serialized_data;
}

std::vector<uint8_t> serialize(const char input) {
    char_tot ++;
    return {static_cast<uint8_t>(input)};
}

std::vector<uint8_t> serialize(const uint16_t input) {
    std::vector<uint8_t> serialized_data(sizeof(input));
    std::memcpy(serialized_data.data(), &input, sizeof(input));
    return serialized_data;
}

// std::string deserialize_string(Compressed& compressed) {
//     if (compressed.compressed_data.size() - compressed.cur < sizeof(uint32_t)) throw std::runtime_error("Not enough data to deserialize string");

//     uint32_t size;
//     std::memcpy(&size, compressed.getNowHeader(), sizeof(size));

//     if (compressed.compressed_data.size() - compressed.cur  < sizeof(size) + size) throw std::runtime_error("Not enough data to deserialize string");

//     std::string result(compressed.getNowHeader() + sizeof(size), compressed.getNowHeader() + sizeof(size) + size);
//     compressed.moveCur(sizeof(size) + size);
//     return result;
// }

int deserialize_int(Compressed& compressed) {
    if (compressed.compressed_data.size() - compressed.cur < sizeof(int)) throw std::runtime_error("Not enough data to deserialize int");

    int result;
    std::memcpy(&result, compressed.getNowHeader(), sizeof(int));
    compressed.moveCur(sizeof(int));
    return result;
}

// 反序列化字符
uint8_t deserialize_uint8_t(Compressed& compressed) {
    if (compressed.compressed_data.size() - compressed.cur < sizeof(uint8_t)) throw std::runtime_error("Not enough data to deserialize char");

    uint8_t result = static_cast<uint8_t>(*compressed.getNowHeader());
    compressed.moveCur(sizeof(uint8_t));

    return result;
}

uint16_t deserialize_uint16_t(Compressed& compressed) {
    if (compressed.compressed_data.size() - compressed.cur < sizeof(uint16_t)) throw std::runtime_error("Not enough data to deserialize uint8_t");
    uint16_t result;
    std::memcpy(&result, compressed.getNowHeader(), sizeof(uint16_t));
    compressed.moveCur(sizeof(uint16_t));
    return result;
}