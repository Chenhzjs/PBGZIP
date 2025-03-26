#ifndef _DEFLATE_H_
#define _DEFLATE_H_
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <bitset>
#include <cstdint>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#define MAX_WINDOW_SIZE 511
#define MAX_LOOKAHEAD_SIZE 20



struct LZ77Token {
    int offset;
    int length;
    uint8_t next_char;
    LZ77Token(int o, int l, uint8_t c) : offset(o), length(l), next_char(c) {}
};

struct ParallelLZ77 {
    int now_thread;
    int token_seq;
    LZ77Token token;
    ParallelLZ77(int n, int t, LZ77Token tok) : now_thread(n), token_seq(t), token(tok) {}
};
struct HuffmanNode {
    uint16_t data;
    int freq;
    HuffmanNode *left, *right;
    HuffmanNode(uint16_t d, int f) : data(d), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

void lz77_compress(const std::string& input, int now_thread, int window_size = MAX_WINDOW_SIZE, int lookahead_size = MAX_LOOKAHEAD_SIZE);

std::string lz77_decompress(const std::vector<LZ77Token>& compressed);

void generateHuffmanCodes(HuffmanNode* root, std::string code, std::unordered_map<uint16_t, std::string>& huffmanTable);

std::vector<std::pair<uint16_t, std::string>>huffman_compress(const std::vector<uint16_t>& input);

std::vector<uint8_t> gzip_compress(const std::string& input, int thread_num);

std::string gzip_decompress(const std::vector<uint8_t>& compressed_data);

extern tbb::concurrent_vector<ParallelLZ77> lz77_concurrent_data;

#endif