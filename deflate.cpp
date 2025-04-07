#include "deflate.h"
#include <algorithm>
#include <execution>
#include <omp.h>
#include <chrono>
// KMP algorithm for pattern matching
std::string lz_out;

// std::vector<LZ77Token> lz77_data;
std::vector<int> lz77_num_per_thread(128);
std::vector<uint8_t> compressed_data;



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
    int i = 0;
    /*
    |max(0, i - window_size)|<-- window_size -->|i|<-- lookahead_size -->|
    */
    std::vector<LZ77Token> lz77_data;
    while (i < input.size()) {
        
        int start = std::max(0, i - window_size);
        std::string window = input.substr(start, i - start);
        std::string lookahead = input.substr(i, lookahead_size);
        int best_length = 0;
        int best_offset = 0;
        uint8_t next_char = '\0';
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
        LZ77Token token(best_offset, best_length, next_char);
        lz77_data.push_back(token);
        if (best_length == 0) {
            i += best_length + 1;
            // std::cout << "next_char: " << next_char << std::endl;
        }
        else i += best_length;
    }
    return lz77_data;
}


std::string lz77_decompress(const std::vector<LZ77Token>& compressed) {
    std::string decompressed;
    int i = 0;
    // std::cout << "compressed size: " << compressed.size() << std::endl;
    for (const auto& token : compressed) {
        int start = decompressed.size() - token.offset;
        for (int j = 0; j < token.length; j ++) {
            decompressed += decompressed[start + j];
        }
        if (token.next_char != '\0' || i != compressed.size() - 1) {
            if (token.length == 0) {
                decompressed += token.next_char;
                // std::cout << "decompressed: " << token.next_char << std::endl;
            }
        }
        i ++;
    }
    // std::cout << "Decompressed " << compressed.size() << " tokens into " << decompressed.size() << " bytes" << std::endl;
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
        for (int i = length - 1; i >= 0; i --) {
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
    for (uint16_t c : input) {
        freq[c] ++;
    }

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
    std::vector<LZ77Token> lz77_data = lz77_compress(input);
    std::vector<uint8_t> huffman_encoded;
    std::vector<uint16_t> char_length_huff;
    std::vector<uint16_t> offset_huff;
    std::vector<uint8_t> tmp_vec;
    
    auto start = std::chrono::high_resolution_clock::now();
    // int tot = 0;
    for (const auto& token : lz77_data) {
        // std::cout << thread_token.now_thread << " <" << thread_token.token.offset << ", " << thread_token.token.length << ", " << thread_token.token.next_char << ">" << std::endl;
        if (token.length > 0) {
            char_length_huff.push_back(static_cast<uint16_t>(token.length + 256));
            offset_huff.push_back(static_cast<uint16_t>(token.offset));
        } else {
            char_length_huff.push_back(static_cast<uint16_t>(token.next_char));
            // std::cout << "next_char: " << token.next_char << std::endl;
        }
    }
    // std::cout << "tot: " << tot << std::endl;
    std::vector<std::pair<uint16_t, std::string>> char_huffman_table = huffman_compress(char_length_huff);
    std::unordered_map<uint16_t, std::string> char_huffman_map;
    // std::cout << "char_huffman_table size: " << char_huffman_table.size() << std::endl;
    for (const auto& pair : char_huffman_table) {
        char_huffman_map[pair.first] = pair.second;
        // std::cout << "char_huffman_table: " << pair.first << " " << pair.second << std::endl;
    }
    std::vector<std::pair<uint16_t, std::string>> offset_huffman_table = huffman_compress(offset_huff);
    std::unordered_map<uint16_t, std::string> offset_huffman_map;
    // std::cout << "offset_huffman_table size: " << offset_huffman_table.size() << std::endl;
    for (const auto& pair : offset_huffman_table) {
        offset_huffman_map[pair.first] = pair.second;
    }
    
    // write char_huffman_table
    tmp_vec = serialize(static_cast<int>(char_huffman_table.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    for (const auto& pair : char_huffman_table) {
        tmp_vec = serialize(pair.first);
        huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
        uint8_t length = pair.second.length();
        huffman_encoded.push_back(length);
        // std::cout << "char_huffman_table: " << pair.first << " " << static_cast<int>(length) << std::endl;
    }
    // write offset_huffman_table
    tmp_vec = serialize(static_cast<int>(offset_huffman_table.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    for (const auto & pair : offset_huffman_table) {
        tmp_vec = serialize(pair.first);
        huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
        uint8_t length = pair.second.length();
        huffman_encoded.push_back(length);
    }
    // std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;



    // encode char_huff
    std::string bit_string_char;
    for (const auto& c : char_length_huff) {
        bit_string_char += char_huffman_map[c];
        // std::cout << "char_huffman_map: " << c << " " << char_huffman_map[c] << std::endl;
    }
    // for (int i = 0; i < bit_string_char.size(); i ++) {
    //     std::cout << bit_string_char[i];
    //     if (i % 8 == 7) {
    //         std::cout << " ";
    //     }
    // }
    // std::cout << std::endl;
    tmp_vec = serialize(static_cast<int>(char_length_huff.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    // std::cout << "char_huff size: " << char_length_huff.size() << std::endl;
    int tmp = 0;
    // std::cout << bit_string_char.size() << " " << bit_string_char.size() / 8 + 1<< std::endl;

    for (int i = 0; i < bit_string_char.size(); i += 8) {
        uint8_t byte = 0;
        int j;
        for (j = 0; j < 8 && i + j < bit_string_char.size(); j ++) {
            byte = (byte << 1) | (bit_string_char[i + j] - '0');
        }
        if (i + j == bit_string_char.size()) {
            byte = byte << (8 - j);
        }
        huffman_encoded.push_back(byte);
        // std::cout << static_cast<int>(byte) << std::endl;
        // tmp ++;
    }
    // std::cout << "tmp: " << tmp << std::endl;
    // std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;
    // encode offset_offset_huff
    std::string bit_string_offset;
    for (const auto& c : offset_huff) {
        bit_string_offset += offset_huffman_map[c];
    }
    tmp_vec = serialize(static_cast<int>(offset_huff.size()));
    huffman_encoded.insert(huffman_encoded.end(), tmp_vec.begin(), tmp_vec.end());
    // std::cout << "offset_huff size: " << offset_huff.size() << std::endl;

    for (int i = 0; i < bit_string_offset.size(); i += 8) {
        uint8_t byte = 0;
        int j;
        for (j = 0; j < 8 && i + j < bit_string_offset.size(); j ++) {
            byte = (byte << 1) | (bit_string_offset[i + j] - '0');
        }
        if (i + j == bit_string_offset.size()) {
            byte = byte << (8 - j);
        }
        huffman_encoded.push_back(byte);
    }
    // std::cout << "huffman_encoded size: " << huffman_encoded.size() << std::endl;
    auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start);
    // std::cout << "time: " << duration.count() / 1000000000.0 << " s" << std::endl;
    // std::cout << "Compressed " << input.size() << " bytes into " << huffman_encoded.size() << " bytes" << std::endl;
    return huffman_encoded;
}

std::string binary_add_one(const std::string& binary) {
    std::string result = binary;
    int carry = 1;  

    for (int i = result.size() - 1; i >= 0; i--) {
        if (result[i] == '0') {
            result[i] = '1';
            carry = 0;  
            break;
        } else {
            result[i] = '0';  
        }
    }

    if (carry) {
        result = "1" + result;  
    }

    return result;
}

std::string binary_left_shift(const std::string& binary) {
    return binary + "0";
}

void get_canonical_codes(std::vector<std::pair<uint16_t, uint8_t>>& codes, std::unordered_map<std::string, uint16_t>& huffman_map) {
    int i = 0;
    int last_length = -1;
    // std::cout << "codes size: " << codes.size() << std::endl;
    std::string code;
    while (i < codes.size()) {
        uint16_t symbol = codes[i].first;
        uint8_t length = codes[i].second;
        if (last_length == -1) {
            for (int j = 0; j < length; j ++) {
                code += "0"; 
            }
            // std::cout << "code: " << static_cast<int>(symbol) << " " << code << std::endl;
        } else {
            if (length > last_length) {
                code = binary_left_shift(binary_add_one(code));
                // std::cout << "code: " << static_cast<int>(symbol) << " " << code << std::endl;
            } else {
                code = binary_add_one(code);
                // std::cout << "code: " << static_cast<int>(symbol) << " " << code << std::endl;
            }
        }
        huffman_map[code] = symbol;
        i += 1;
        last_length = length;
    }
}

std::vector<DepressedBlockData*> gzip_decompress(const std::string input_name, int thread_num) {
    // std::vector<uint8_t> data = compressed_data;
    std::vector<int> start_indexes;
    std::vector<int> end_indexes;
    int size = std::filesystem::file_size(input_name);
    int batch_size = size / thread_num;
    std::ifstream in(input_name, std::ios::binary);
    for (int i = 0; i < thread_num; i++) {
        in.seekg(i * batch_size);
        start_indexes.push_back(findNextMagicNumber(in));
        if (i == 0) {

        } else {
            end_indexes.push_back(start_indexes[i]);
        }
    }
    end_indexes.push_back(size);
    in.close();
    std::vector<DepressedBlockData*> blocks;
    for (int i = 0; i < thread_num; i ++) {
        blocks.push_back(new DepressedBlockData(i));
    }
    #pragma omp parallel for num_threads(thread_num) schedule(static)
    for (int i = 0; i < thread_num; i ++) {
        std::ifstream local_in(input_name, std::ios::binary);
        local_in.seekg(start_indexes[i]);
        int buffer_size = end_indexes[i] - start_indexes[i];
        std::vector<uint8_t> compressed_data(buffer_size);
        local_in.read(reinterpret_cast<char*>(compressed_data.data()), buffer_size);

        while (compressed_data.size() > 0) {
            // std::cout << "block_data size: " << block_data.size() << std::endl;
            std::vector<LZ77Token> lz77_encoded;
            std::vector<uint8_t> data = blocks[i]->get_raw_data(compressed_data);
            int char_length_huff_size = deserialize_int(data);
            // std::cout << "char_huff_size: " << char_length_huff_size << std::endl;
            std::vector<std::pair<uint16_t, uint8_t>> char_huffman_table;
            for (int i = 0; i < char_length_huff_size; i++) {
                uint16_t symbol = deserialize_uint16_t(data);
                uint8_t length = deserialize_uint8_t(data);
                char_huffman_table.push_back({symbol, length});
                // std::cout << "char_huffman_table: " << static_cast<int>(symbol) << " " << static_cast<int>(length) << std::endl;
            }

            int offset_huff_size = deserialize_int(data);
            // std::cout << "offset_huff_size: " << offset_huff_size << std::endl;
            std::vector<std::pair<uint16_t, uint8_t>> offset_huffman_table;
            for (int i = 0; i < offset_huff_size; i++) {
                uint16_t symbol = deserialize_uint16_t(data);
                uint8_t length = deserialize_uint8_t(data);
                offset_huffman_table.push_back({symbol, length});
            }
            std::unordered_map<std::string, uint16_t> char_length_huffman_map;
            std::unordered_map<std::string, uint16_t> offset_huffman_map;
            get_canonical_codes(char_huffman_table, char_length_huffman_map);
            // for (const auto& pair : char_length_huffman_map) {
            //     std::cout << "char_length_huffman_map: " << pair.second << " " << pair.first << std::endl;
            // }
            // std::cout << "char_length_huffman_map size: " << char_length_huffman_map.size() << std::endl;
            get_canonical_codes(offset_huffman_table, offset_huffman_map);
            // std::cout << "offset_huffman_map size: " << offset_huffman_map.size() << std::endl;


            int bit_string_char_size = deserialize_int(data);
            // std::cout << "bit_string_char_size: " << bit_string_char_size << std::endl;
            std::vector<uint16_t> bit_string_char_vec;
            std::string bit_string_char = "";
            int count = 0;
            int tmp = 0;
            while (1) {
                uint8_t byte = deserialize_uint8_t(data);
                // std::cout << bit_string_char << " " << static_cast<int>(byte) << std::endl;
                // tmp ++;
                for (int i = 7; i >= 0; i --) {
                    bit_string_char = bit_string_char + ((byte >> i) & 1 ? "1" : "0");
                    if (char_length_huffman_map.find(bit_string_char) != char_length_huffman_map.end()) {
                        // std::cout << "number = " << char_length_huffman_map[bit_string_char] << std::endl;
                        bit_string_char_vec.push_back(char_length_huffman_map[bit_string_char]);
                        bit_string_char = "";
                        count ++;
                        // std::cout << "count: " << count << std::endl;
                    } 
                    if (count == bit_string_char_size) {
                        break;
                    }
                }
                if (count == bit_string_char_size) {
                    break;
                }
            }
            // std::cout << "tmp: " << tmp << std::endl;
            int bit_string_offset_size = deserialize_int(data);
            // std::cout << "bit_string_offset_size: " << bit_string_offset_size << std::endl;
            std::vector<uint16_t> bit_string_offset_vec;
            std::string bit_string_offset = "";
            count = 0;
            while(1) {
                uint8_t byte = deserialize_uint8_t(data);
                for (int i = 7; i >= 0; i --) {
                    bit_string_offset = bit_string_offset + ((byte >> i) & 1 ? "1" : "0");
                    if (offset_huffman_map.find(bit_string_offset) != offset_huffman_map.end()) {
                        bit_string_offset_vec.push_back(offset_huffman_map[bit_string_offset]);
                        bit_string_offset = "";
                        count ++;
                    }
                    if (count == bit_string_offset_size) {
                        break;
                    }
                }
                if (count == bit_string_offset_size) {
                    break;
                }
            }

            // std::cout << "bit_string_offset_vec size: " << bit_string_offset_vec.size() << std::endl;
            // int tot = 0;
            for (auto c : bit_string_char_vec) {
                // std::cout << c << " ";
                int length = 0;
                int offset = 0;
                uint8_t next_char;
                if (c < 256) {
                    next_char = static_cast<uint8_t>(c);
                    // std::cout << "next_char: " << next_char << std::endl;
                    // tot ++;
                    length = 0;
                    offset = 0;
                } else {
                    length = static_cast<int>(c) - 256;
                    if (length > 20) {
                        std::cout << "length: " << length << std::endl;
                        throw std::runtime_error("Invalid length");
                    }
                    // tot += length;
                    offset = bit_string_offset_vec[0];
                    bit_string_offset_vec.erase(bit_string_offset_vec.begin());
                    // std::cout << "length: " << length << " offset: " << offset << std::endl;
                    // next_char = static_cast<char>(bit_string_char_vec[0]);
                }
                lz77_encoded.push_back({offset, length, next_char});
            }

            std::string lz77_data = lz77_decompress(lz77_encoded);
            blocks[i]->insert(lz77_data);
        }
        local_in.close();
    }
    return blocks;
}


int findNextMagicNumber(std::ifstream& in) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    std::streampos initialPosition;
    
    while (in.good()) {
        initialPosition = in.tellg();
        in.read(buffer, BUFFER_SIZE);
        int bytesRead = in.gcount();
        
        if (bytesRead == 0) break;
        
        for (int i = 0; i < bytesRead - 4; i++) {
            if (static_cast<unsigned char>(buffer[i]) == 0x5A && 
                static_cast<unsigned char>(buffer[i+1]) == 0x4C && 
                static_cast<unsigned char>(buffer[i+2]) == 0x47 && 
                static_cast<unsigned char>(buffer[i+3]) == 0x50 && 
                static_cast<unsigned char>(buffer[i+4]) == 0x01) {
                return static_cast<int>(initialPosition) + i;
            }
        }
        
        if (bytesRead >= 5) {
            in.seekg(-5, std::ios::cur);
        }
    }
    return -1;
}
