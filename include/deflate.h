#ifndef _DEFLATE_H_
#define _DEFLATE_H_
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <bitset>

struct LZ77Token {
    int offset;
    int length;
    char next_char;
};


struct HuffmanNode {
    char data;
    int freq;
    HuffmanNode *left, *right;
    HuffmanNode(char d, int f) : data(d), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

std::vector<LZ77Token> lz77_compress(const std::string& input, int window_size = 1024, int lookahead_size = 32);

std::string lz77_decompress(const std::vector<LZ77Token>& compressed);

void generateHuffmanCodes(HuffmanNode* root, std::string code, std::unordered_map<char, std::string>& huffmanTable);

std::unordered_map<char, std::string> huffman_compress(const std::string& input);

std::vector<uint8_t> gzip_compress(const std::string& input);

std::string gzip_decompress(const std::vector<uint8_t>& compressed_data);

#endif