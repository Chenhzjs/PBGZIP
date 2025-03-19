#include "deflate.h"
#include "serialize.h"
#include <algorithm>
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
int find_last_match(const std::string& text, const std::string& pattern) {
    if (pattern.empty() || text.empty() || pattern.size() > text.size()) 
        return -1;

    std::string reversed_text(text.rbegin(), text.rend());
    std::string reversed_pattern(pattern.rbegin(), pattern.rend());

    int pos = kmp(reversed_text, reversed_pattern);
    
    return (pos == -1) ? -1 : (text.size() - pos - pattern.size());
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
        for (int j = 2; j < lookahead.size() && j < window.size(); j ++) {
            int matches = find_last_match(window, lookahead.substr(0, j + 1));
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
        if (best_length == 0) i += best_length + 1;
        else i += best_length;
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
            if (token.length != 0) {
                decompressed += token.next_char;
            }
        }
        i ++;
    }
    std::cout << "Decompressed " << compressed.size() << " tokens into " << decompressed.size() << " bytes" << std::endl;
    return decompressed;
}

void generateHuffmanCodes(HuffmanNode* node, std::string code, std::unordered_map<uint16_t, std::string>& huffman_table) {
    if (!node) return;
    if (!node->left && !node->right) {
        // std::cout << node->data << " " << code << std::endl;
        huffman_table[node->data] = code;
    }
    generateHuffmanCodes(node->left, code + "0", huffman_table);
    generateHuffmanCodes(node->right, code + "1", huffman_table);
}

std::vector<std::pair<uint16_t, std::string>> build_canonical_codes(
    const std::vector<std::pair<int, uint16_t>>& sorted_symbols) 
{
    std::vector<std::pair<uint16_t, std::string>> codes;
    int current_code = 0;
    int prev_length = 0;

    for (const auto& pair: sorted_symbols) {
        uint16_t symbol = pair.second;
        int length = pair.first;
        if (prev_length != 0) {
            current_code = (current_code + 1) << (length - prev_length);
        }
        
        std::string code;
        for (int i = length-1; i >= 0; --i) {
            code += ((current_code >> i) & 1) ? '1' : '0';
        }
        
        codes.push_back({symbol, code});
        prev_length = length;
    }
    return codes;
}
bool cmp(const std::pair<int, uint16_t>& a, const std::pair<int, uint16_t>& b) {
    if (a.first == b.first) {
        return a.second < b.second;
    }
    return a.first < b.first;
}

std::vector<std::pair<uint16_t, std::string>> huffman_compress(const std::vector<uint16_t>& input) {

    std::unordered_map<uint16_t, int> freq;
    for (uint16_t c : input) freq[c]++;

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> pq;
    for (auto& pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        auto node = new HuffmanNode('\0', left->freq + right->freq);
        node->left = left;
        node->right = right;
        pq.push(node);
    }

    std::unordered_map<uint16_t, std::string> huffman_table;


    generateHuffmanCodes(pq.top(), "", huffman_table);

    std::vector<std::pair<int, uint16_t>> symbols_with_length;
    for (const auto& pair : huffman_table) {
        uint16_t symbol = pair.first;
        const std::string& code = pair.second;
        symbols_with_length.emplace_back(code.length(), symbol);
    }

    std::sort(symbols_with_length.begin(), symbols_with_length.end(), cmp);
    

    return build_canonical_codes(symbols_with_length);
}



std::vector<uint8_t> gzip_compress(const std::string& input) {
    std::vector<LZ77Token> lz77_encoded = lz77_compress(input);
    std::vector<uint8_t> huffman_encoded;
    std::vector<uint16_t> char_length_huff;
    std::vector<uint16_t> offset_huff;
    std::vector<uint8_t> tmp_vec;
    for (const auto& token : lz77_encoded) {
        char_length_huff.push_back(static_cast<uint16_t>(token.next_char));
        if (token.length != 0) {
            char_length_huff.push_back(static_cast<uint16_t>(token.length + 256));
        }
        if (token.length != 0) {
            offset_huff.push_back(static_cast<uint16_t>(token.offset));
        }
        
    }
    std::vector<std::pair<uint16_t, std::string>> char_huffman_table = huffman_compress(char_length_huff);
    std::unordered_map<uint16_t, std::string> char_huffman_map;
    std::cout << "char_huffman_table size: " << char_huffman_table.size() << std::endl;
    for (const auto& pair : char_huffman_table) {
        char_huffman_map[pair.first] = pair.second;
    }
    std::vector<std::pair<uint16_t, std::string>> offset_huffman_table = huffman_compress(offset_huff);
    std::unordered_map<uint16_t, std::string> offset_huffman_map;
    std::cout << "offset_huffman_table size: " << offset_huffman_table.size() << std::endl;
    for (const auto& pair : offset_huffman_table) {
        offset_huffman_map[pair.first] = pair.second;
    }
    
    // write char_huffman_table
    huffman_encoded.push_back(char_huffman_table.size());
    for (const auto& pair : char_huffman_table) {
        huffman_encoded.push_back(pair.first);
        uint8_t length = pair.second.length();
        huffman_encoded.push_back(length);
    }
    // write offset_huffman_table
    huffman_encoded.push_back(offset_huffman_table.size());
    for (const auto & pair : offset_huffman_table) {
        huffman_encoded.push_back(pair.first);
        uint8_t length = pair.second.length();
        huffman_encoded.push_back(length);
    }
    
    std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;
    // encode char_huff
    std::string bit_string_char;
    for (const auto& c : char_length_huff) {
        bit_string_char += char_huffman_map[c];
    }
    tmp_vec = serialize(static_cast<int>(bit_string_char.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    std::cout << "char_huff size: " << char_length_huff.size() << std::endl;
    for (int i = 0; i < bit_string_char.size(); i += 8) {
        uint8_t byte = 0;
        for (int j = 0; j < 8 && i + j < bit_string_char.size(); j++) {
            byte = (byte << 1) | (bit_string_char[i + j] - '0');
        }
        huffman_encoded.push_back(byte);
    }
    std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;
    // encode offset_offset_huff
    std::string bit_string_offset;
    for (const auto& c : offset_huff) {
        bit_string_offset += offset_huffman_map[c];
    }
    tmp_vec = serialize(static_cast<int>(bit_string_offset.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    std::cout << "offset_huff size: " << offset_huff.size() << std::endl;
    for (int i = 0; i < bit_string_offset.size(); i += 8) {
        uint8_t byte = 0;
        for (int j = 0; j < 8 && i + j < bit_string_offset.size(); j++) {
            byte = (byte << 1) | (bit_string_offset[i + j] - '0');
        }
        huffman_encoded.push_back(byte);
    }
    std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;
    std::cout << "Compressed " << input.size() << " bytes into " << huffman_encoded.size() << " bytes" << std::endl;
    return huffman_encoded;
}

std::string gzip_decompress(const std::vector<uint8_t>& compressed_data) {
    std::vector<LZ77Token> lz77_encoded;
    
    std::cout << "Decompressed " << compressed_data.size() << " bytes into " << lz77_encoded.size() << " tokens" << std::endl;
    return lz77_decompress(lz77_encoded);
}

