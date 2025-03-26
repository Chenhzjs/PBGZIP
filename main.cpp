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
std::vector<uint8_t> stringToU8(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
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



int main(int argc, char* argv[]) {
    // std::string text = "abcabcabcabcabcabcabcabcabc";
    // std::string text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaababfbadbfdbadfsadfdsafadsfdsafdasfdsfewgjfkijvoasdjoas";
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file> <thread num> <-c/-x>" << std::endl;
        return 1;
    }
    std::string input = read(argv[1]);
    std::string output = argv[2];
    int thread_num = std::stoi(argv[3]);
    std::string mode = argv[4];
    // std::string input = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";
    // std::vector<LZ77Token> compressed = lz77_compress(text);
    if (mode == "-c") {
        std::vector<uint8_t> compressed_data = gzip_compress(input, thread_num);
        write(output, std::string(compressed_data.begin(), compressed_data.end()));
    }
    else if (mode == "-x") {
        std::string compressed_data = read(argv[1]);
        std::vector<uint8_t> compressed_data_u8 = stringToU8(compressed_data);
        std::string decompressed_data = gzip_decompress(compressed_data_u8);
        write(output, decompressed_data);
    }
    else {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file> <thread num> <-c/-x>" << std::endl;
        return 1;
    }
    return 0;
}