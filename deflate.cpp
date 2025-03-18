#include "deflate.h"
#include "serialize.h"
// KMP algorithm for pattern matching
std::string lz_out;
std::vector<LZ77Token> lz77_data;
int byte_tot = 0;
int kmp(const std::string& text, const std::string& pattern) {
    std::vector<int> lps(pattern.size(), 0);
    int matches = -1;

    // Preprocess the pattern to calculate the longest proper prefix which is also a suffix
    int i = 1, len = 0;
    while (i < pattern.size()) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }

    // Perform pattern matching using the KMP algorithm
    int j = 0;
    i = 0;
    while (i < text.size()) {
        if (pattern[j] == text[i]) {
            j++;
            i++;
        }

        if (j == pattern.size()) {
            matches = i - j;
            break; // Stop after finding the first match
        } else if (i < text.size() && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }

    return matches;
}

std::vector<LZ77Token> lz77_compress(const std::string& input, int window_size, int lookahead_size) {
    std::vector<LZ77Token> compressed;
    int i = 0;
    /*
    |max(0, i - window_size)|<-- window_size -->|i|<-- lookahead_size -->|
    */
    while (i < input.size()) {
        int start = std::max(0, i - window_size);
        std::string window = input.substr(start, i - start);
        std::string lookahead = input.substr(i, lookahead_size);
        int best_length = 0;
        int best_offset = 0;
        char next_char = '\0';
        for (int j = 0; j < lookahead.size() && j < window.size(); j ++) {
            int matches = kmp(window, lookahead.substr(0, j + 1));
            // std::cout << window << " " << lookahead.substr(0, j + 1) << " " << matches << std::endl;
            if (matches == -1) {
                break;
            } else {
                best_length = j + 1;
                best_offset = window.size() - matches;

            }
        }
        if (i + best_length < input.size()) {
            next_char = input[i + best_length];
        }
        compressed.push_back({best_offset, best_length, next_char});
        i += best_length + 1;
    }
    lz77_data = compressed;
    std::cout << "Compressed " << input.size() << " bytes into " << compressed.size() << " tokens" << std::endl;
    return compressed;
}


std::string lz77_decompress(const std::vector<LZ77Token>& compressed) {
    std::string decompressed;
    int i = 0;
    for (const auto& token : compressed) {
        int start = decompressed.size() - token.offset;
        for (int j = 0; j < token.length; j ++) {
            decompressed += decompressed[start + j];
        }
        if (token.next_char != '\0' || i != compressed.size() - 1) {
            decompressed += token.next_char;
        }
        i ++;
    }
    std::cout << "Decompressed " << compressed.size() << " tokens into " << decompressed.size() << " bytes" << std::endl;
    return decompressed;
}


void generateHuffmanCodes(HuffmanNode* root, std::string code, std::unordered_map<char, std::string>& huffman_table) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffman_table[root->data] = code;
    }
    generateHuffmanCodes(root->left, code + "0", huffman_table);
    generateHuffmanCodes(root->right, code + "1", huffman_table);
}


std::unordered_map<char, std::string> huffman_compress(const std::string& input) {
    std::unordered_map<char, int> freq;
    for (char c : input) freq[c] ++;

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;

    for (auto& pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();
        HuffmanNode* node = new HuffmanNode('\0', left->freq + right->freq);
        node->left = left;
        node->right = right;
        pq.push(node);
    }
    std::unordered_map<char, std::string> huffman_table;
    generateHuffmanCodes(pq.top(), "", huffman_table);
    return huffman_table;
}



std::vector<uint8_t> gzip_compress(const std::string& input) {
    std::vector<LZ77Token> lz77_encoded = lz77_compress(input);
    std::string lz_output;
    std::vector<uint8_t> token_offset_length;
    std::vector<std::string> tokens;
    for (const auto& token : lz77_encoded) {
        std::string token_offset, token_length, token_next_char;
        token_offset = std::to_string(token.offset); 
        token_length = std::to_string(token.length);
        token_next_char = token.next_char;
        token_offset_length.push_back(static_cast<uint8_t>(token_offset.size()));
        token_offset_length.push_back(static_cast<uint8_t>(token_length.size()));
        // std::cout << static_cast<int>(static_cast<uint8_t>(token_offset.size())) << " " << token_length.size() << std::endl;
        tokens.push_back(token_offset + token_length + token_next_char);
        lz_output += token_offset + token_length + token_next_char;
    }

    std::unordered_map<char, std::string> huffman_table = huffman_compress(lz_output);
    // for (const auto& pair : huffman_table) {
    //     std::cout << pair.first << " " << pair.second << std::endl;
    // }
    std::vector<uint8_t> huffman_encoded;
    std::vector<uint8_t> table_size = serialize(static_cast<int>(huffman_table.size()));
    huffman_encoded.insert(huffman_encoded.end(), table_size.begin(), table_size.end());
    for (const auto& pair : huffman_table) {
        std::vector<uint8_t> serialized_char = serialize(pair.first);
        std::vector<uint8_t> serialized_string = serialize(pair.second);
        // std::cout << pair.first << " " << pair.second << std::endl;
        huffman_encoded.insert(huffman_encoded.end(), serialized_string.begin(), serialized_string.end());
        huffman_encoded.insert(huffman_encoded.end(), serialized_char.begin(), serialized_char.end());
    }
    std::string bit_string;
    std::vector<uint8_t> tokens_size = serialize(static_cast<int>(tokens.size()));
    huffman_encoded.insert(huffman_encoded.end(), tokens_size.begin(), tokens_size.end());
    // int k = 0;
    for (int i = 0; i < tokens.size(); i ++) {
        huffman_encoded.push_back(token_offset_length[i * 2]);
        huffman_encoded.push_back(token_offset_length[i * 2 + 1]);
        // std::cout << token_offset_length[i * 2] << " " << token_offset_length[i * 2 + 1] << std::endl;
        for (char token : tokens[i]) {
            bit_string += huffman_table[token];
            // k ++;
        }
    }
    std::cout << "lz_output: " << lz_output.size() << std::endl;
    // std::cout << "k: " << k << std::endl;
    lz_out = lz_output;
    // std::cout << "lz_output: " << lz_output << std::endl;
    // std::cout << "bit_string_size: " << bit_string.size() << std::endl;
    std::vector<uint8_t> lz_output_size = serialize(static_cast<int>(lz_output.size()));
    huffman_encoded.insert(huffman_encoded.end(), lz_output_size.begin(), lz_output_size.end());
    for (size_t i = 0; i < bit_string.size(); i += 8) {
        uint8_t byte = 0;
        for (size_t j = 0; j < 8 && (i + j) < bit_string.size(); ++ j) {
            if (bit_string[i + j] == '1') {
                byte |= (1 << (7 - j));  // 从高位到低位填充
            }
        }
        byte_tot ++;
        huffman_encoded.push_back(byte);
    }
    // for (auto encoded : huffman_encoded) {
    //     std::cout << encoded << " ";
    // }
    // std::cout << std::endl;
    std::cout << "Compressed " << input.size() << " bytes into " << huffman_encoded.size() << " bytes" << std::endl;
    return huffman_encoded;
}

std::string gzip_decompress(const std::vector<uint8_t>& compressed_data) {
    std::unordered_map<std::string, char> huffman_table;
    std::vector<uint8_t> compressed_data_copy = compressed_data;
    std::vector<int> token_offset_length;
    int table_size = deserialize_int(compressed_data_copy);
    for (int i = 0; i < table_size; i ++) {
        std::string code = deserialize_string(compressed_data_copy);
        char c = deserialize_char(compressed_data_copy);
        huffman_table[code] = c;
    }

    int tokens_num = deserialize_int(compressed_data_copy);

    for (int i = 0; i < tokens_num; i ++) {
        token_offset_length.push_back(static_cast<int>(deserialize_uint8_t(compressed_data_copy)));
        token_offset_length.push_back(static_cast<int>(deserialize_uint8_t(compressed_data_copy)));
        // std::cout << token_offset_length[i * 2] << " " << token_offset_length[i * 2 + 1] << std::endl;
    }


    std::string bit_string;
    int lz_output_size = deserialize_int(compressed_data_copy);
    for (uint8_t byte : compressed_data_copy) {
        for (int i = 7; i >= 0; i --) {
            bit_string += ((byte >> i) & 1) ? '1' : '0';
        }
    }

    std::string lz_output;
    std::string code;
    int t = 0;
    for (char c : bit_string) {
        code += c;
        if (huffman_table.count(code)) {
            if (t ++ >= lz_output_size) break;

            lz_output += huffman_table[code];
            // std::cout << code << " " << huffman_table[code] << std::endl;
            code = "";
        }
    }
    // std::cout << "lz_output: " << lz_output.substr(0, 100) << std::endl;
    // for (int i = 0; i < lz_out.size(); i ++) {
    //     if (lz_out[i] != lz_output[i]) {
    //         std::cout << lz_out[i] << " " << lz_output[i] << std::endl;
    //         std::cout << "error" << std::endl;
    //         break;
    //     }
    // }
    // std::cout << "lz_output is correct" << std::endl;
    std::vector<LZ77Token> lz77_encoded;
    size_t start = 0;
    // exit(1);
    int token_t = 0;
    while (start < lz_output.size()) {
        int offset = std::stoi(lz_output.substr(start, token_offset_length[token_t * 2]));
        start += token_offset_length[token_t * 2];
        int length = std::stoi(lz_output.substr(start, token_offset_length[token_t * 2 + 1]));
        start += token_offset_length[token_t * 2 + 1];

        char next_char = lz_output[start];
        // if (start == 2) {
        //     std::cout << offset << " " << length << " " << next_char << std::endl;
        // }
        start ++;
        lz77_encoded.push_back({offset, length, next_char});
        token_t ++;
    }
    int m = 0;
    // for (auto token : lz77_data) {
    //     if (token.offset != lz77_encoded[m].offset || token.length != lz77_encoded[m].length || token.next_char != lz77_encoded[m].next_char) {
    //         std::cout << m << std::endl;
    //         std::cout << token.offset << " | " << lz77_encoded[m].offset << std::endl;
    //         std::cout << token.length << " | " << lz77_encoded[m].length << std::endl;
    //         std::cout << token.next_char << " | " << lz77_encoded[m].next_char << std::endl;
    //         std::cout << "error" << std::endl;
    //         exit(1);
    //     }
    //     m ++;

    // }
    std::cout << "Decompressed " << lz_output.size() << " bytes into " << lz77_encoded.size() << " tokens" << std::endl;
    return lz77_decompress(lz77_encoded);
}

