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

std::string deserialize_string(std::vector<uint8_t>& compressed_data) {
    if (compressed_data.size() < sizeof(uint32_t)) throw std::runtime_error("Not enough data to deserialize string");

    uint32_t size;
    std::memcpy(&size, compressed_data.data(), sizeof(size));

    if (compressed_data.size() < sizeof(size) + size) throw std::runtime_error("Not enough data to deserialize string");

    std::string result(compressed_data.begin() + sizeof(size), compressed_data.begin() + sizeof(size) + size);
    compressed_data.erase(compressed_data.begin(), compressed_data.begin() + sizeof(size) + size);
    return result;
}

int deserialize_int(std::vector<uint8_t>& compressed_data) {
    if (compressed_data.size() < sizeof(int)) throw std::runtime_error("Not enough data to deserialize int");

    int result;
    std::memcpy(&result, compressed_data.data(), sizeof(result));
    compressed_data.erase(compressed_data.begin(), compressed_data.begin() + sizeof(result));
    return result;
}

// 反序列化字符
char deserialize_char(std::vector<uint8_t>& compressed_data) {
    if (compressed_data.empty()) throw std::runtime_error("Not enough data to deserialize char");

    char result = static_cast<char>(compressed_data[0]);
    compressed_data.erase(compressed_data.begin());
    return result;
}

uint8_t deserialize_uint8_t(std::vector<uint8_t>& compressed_data) {
    if (compressed_data.empty()) throw std::runtime_error("Not enough data to deserialize uint8_t");

    uint8_t result = compressed_data[0];
    compressed_data.erase(compressed_data.begin());
    return result;
}