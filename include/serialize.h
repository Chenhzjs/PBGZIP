#ifndef _SERIALIZE_H_
#define _SERIALIZE_H_
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

std::vector<uint8_t> serialize(const std::string& input);

std::vector<uint8_t> serialize(const int input);

std::vector<uint8_t> serialize(const char input);

std::vector<uint8_t> serialize(const uint16_t input);

std::string deserialize_string(std::vector<uint8_t>& compressed_data);

int deserialize_int(std::vector<uint8_t>& compressed_data);

uint8_t deserialize_uint8_t(std::vector<uint8_t>& compressed_data);

uint16_t deserialize_uint16_t(std::vector<uint8_t>& compressed_data);

#endif