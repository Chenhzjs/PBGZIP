#include "deflate.h"
#include <iostream>
#include <fstream>


void print_LZ77(const std::vector<LZ77Token>& compressed) {
    for (const auto& token : compressed) {
        std::cout << "<" << token.offset << ", " << token.length << ", " << token.next_char << ">" << std::endl;
    }
}

std::string read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::cout << "Read " << content.size() << " bytes from " << filename << std::endl;
    return content;
}

void write(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }
    file.write(content.data(), content.size());
    file.close();
}

bool diff(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        std::cerr << "Different sizes: " << a.size() << " vs " << b.size() << std::endl;
        return false;
    }
    // std::cout << b[b.size() - 1] << std::endl;
    for (size_t i = 0; i < a.size(); i ++) {
        if (a[i] != b[i]) {
            std::cerr << "Mismatch at position " << i << ": " << a[i] << " vs " << b[i] << std::endl;
            return false;
        }
    }

    return true;
}
int main() {
    // std::string text = "abcabcabcabcabcabcabcabcabc";
    // std::string text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaababfbadbfdbadfsadfdsafadsfdsafdasfdsfewgjfkijvoasdjoas";
    std::string text = read("random_data.txt");
    // std::vector<LZ77Token> compressed = lz77_compress(text);
    std::vector<uint8_t> compressed_data = gzip_compress(text);
    extern int int_tot;
    extern int string_tot;
    extern int char_tot;
    extern int byte_tot;
    std::cout << "int: " << int_tot << std::endl;   
    std::cout << "string: " << string_tot << std::endl;
    std::cout << "char: " << char_tot << std::endl;
    std::cout << "byte: " << byte_tot << std::endl;
    // for (auto c : compressed_data) {

    //     for (int i = 7; i >= 0; i --) {
    //         std::cout << ((c >> i) & 1);
    //     }
    // }
    // std::cout << std::endl;
    // std::string decompressed_data = gzip_decompress(compressed_data);
    // std::cout << text << std::endl;
    // std::cout << decompressed_data << std::endl;
    // write("decompressed.bin", decompressed_data);
    // if (diff(text, decompressed_data)) {
    //     std::cout << "Decompressed data is correct" << std::endl;
    // } else {
    //     std::cerr << "Decompressed data is incorrect" << std::endl;
    // }
    return 0;
}