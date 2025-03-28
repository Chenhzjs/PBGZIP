#ifndef _DEFLATE_H_
#define _DEFLATE_H_
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <bitset>
#include <cstdint>
#include <zlib.h>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include "serialize.h"

#define MAX_WINDOW_SIZE 511
#define MAX_LOOKAHEAD_SIZE 20

constexpr uint32_t MAGIC = 0x50474C5A;
constexpr uint8_t VERSION = 0x01;         // 版本号
constexpr int BLOCK_SIZE = 65536;
struct LZ77Token {
    int offset;
    int length;
    uint8_t next_char;
    LZ77Token(int o, int l, uint8_t c) : offset(o), length(l), next_char(c) {}
};

struct HuffmanNode {
    uint16_t data;
    int freq;
    HuffmanNode *left, *right;
    HuffmanNode(uint16_t d, int f) : data(d), freq(f), left(nullptr), right(nullptr) {}
};



int findNextMagicNumber(const std::vector<uint8_t>& compressed_data, int start);

std::vector<LZ77Token> lz77_compress(const std::string& input, int window_size = MAX_WINDOW_SIZE, int lookahead_size = MAX_LOOKAHEAD_SIZE);

std::string lz77_decompress(const std::vector<LZ77Token>& compressed);

void generateHuffmanCodes(HuffmanNode* root, std::string code, std::unordered_map<uint16_t, std::string>& huffmanTable);

std::vector<std::pair<uint16_t, std::string>> huffman_compress(const std::vector<uint16_t>& input);

std::vector<uint8_t> gzip_compress(const std::string& input);


class CompressedBlockData {
public:
    CompressedBlockData(int now_thread) : now_thread(now_thread) {
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
        std::string filename = std::to_string(now_thread) + ".tmp";
        std::string temp_file = (temp_dir / filename).string();
        file_name = temp_file;
        file.open(temp_file, std::ios::binary);
    }
    ~CompressedBlockData() {
        close();
    }

    void push_back(char byte) {
        data += byte;
        if (data.size() == BLOCK_SIZE) { 
            write();
        }
    }

    void insert(std::string bytes) {
        for (char byte : bytes) {
            push_back(byte);
        }
    }

    void flush() { 
        if (data.size() == 0) return ;
        write();
    }

    std::string get_filename() const {
        return file_name;
    }
    std::vector<uint16_t> char_length_huff;
    std::vector<uint16_t> offset_huff;
private:
    std::vector<uint8_t> deflate() {
        std::vector<uint8_t> compressed_data = gzip_compress(data);
        // std::cout << "Compressed " << data.size() << " bytes into " << compressed_data.size() << " bytes" << std::endl;
        return compressed_data;
    }
    void write() {
        std::vector<uint8_t> compressed_data = deflate();
        
        file.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
        file.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));
        uint16_t block_size = compressed_data.size();
        file.write(reinterpret_cast<const char*>(&block_size), sizeof(block_size));
        file.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
        uint32_t checksum = crc32(0L, Z_NULL, 0);  
        checksum = crc32(checksum, compressed_data.data(), compressed_data.size());
        file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
        file.flush();
        // compress_total += 4 + 1 + 2 + compressed_data.size() + 4;
        data.clear();
    }
    void close() {
        if (data.size() > 0) {
            throw std::runtime_error("Data not flushed");
        }
        // printf("Compressed %d bytes into %d bytes\n", compress_total, file.tellp());
        file.close();
    }
    std::string data;
    // constexpr static int BLOCK_SIZE = 2048 - 4;
    int now_thread = 0;
    std::string file_name;
    std::ofstream file;
    // int compress_total = 0;
};

class DepressedBlockData {
public:
    DepressedBlockData(int now_thread) : now_thread(now_thread) {}
    std::vector<uint8_t> get_raw_data(std::vector<uint8_t>& block_data) {
        // std::vector<uint8_t> data = block_data;
        // std::cout << block_data.size() << std::endl;
        // int before_size = block_data.size();
        int magic = deserialize_int(block_data);
        uint8_t version = deserialize_uint8_t(block_data);
        uint16_t block_size = deserialize_uint16_t(block_data);
        std::vector<uint8_t> raw_data;
        // std::cout << block_data.size() << std::endl;
        for (int i = 0; i < block_size; i ++) {
            raw_data.push_back(deserialize_uint8_t(block_data));
        }
        // std::cout << "now_thread: " << now_thread << " block_size: " << block_size << std::endl;
        int crc = deserialize_int(block_data);
        // int after_size = block_data.size();
        // std::cout << "block_data size: " << before_size - after_size << std::endl;
        int checksum = crc32(0L, Z_NULL, 0);
        checksum = crc32(checksum, raw_data.data(), raw_data.size());
        if (crc != checksum) {
            throw std::runtime_error("Checksum mismatch");
        }
        return raw_data;
    }
    void insert(const std::string& bytes) {
        for (char byte : bytes) {
            push_back(byte);
        }
    }
    void push_back(char byte) {
        depressed_data += byte;
    }
    std::string get_data() const {
        return depressed_data;
    }
private:
    std::string depressed_data;
    int now_thread = 0;
};
struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};
std::vector<DepressedBlockData*> gzip_decompress(const std::vector<uint8_t>& compressed_data, int thread_num);

#endif