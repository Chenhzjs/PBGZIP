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
    // std::cout << "Read " << content.size() << " bytes from " << filename << std::endl;
    return content;
}
std::vector<uint8_t> stringToU8(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}



void write(const std::string& out_filename, std::vector<DepressedBlockData*>& blocks) {
    std::ofstream out_file(out_filename, std::ios::binary | std::ios::trunc);
    if (!out_file) {
        std::cerr << "Error: Unable to open output file " << out_filename << std::endl;
        return;
    }

    for (const auto& block : blocks) {
        std::string content = block->get_data();
        out_file.write(content.data(), content.size());
    }

    out_file.close();
}

void write(const std::string& out_filename, std::vector<CompressedBlockData*>& blocks) {
    std::ofstream out_file(out_filename, std::ios::binary | std::ios::trunc);
    if (!out_file) {
        std::cerr << "Error: Unable to open output file " << out_filename << std::endl;
        return;
    }

    for (const auto& block : blocks) {
        std::ifstream in_file(block->get_filename(), std::ios::binary);
        if (!in_file) {
            std::cerr << "Warning: Unable to open block file " << block->get_filename() << std::endl;
            continue;  
        }

        out_file << in_file.rdbuf();
        in_file.close();
    }

    out_file.close();
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

std::vector<std::string> split(const std::string &text, int n) {
    std::vector<std::string> parts;
    if (text.empty() || n <= 0) return parts; 

    int total_size = text.size();
    int part_size = total_size / n;
    int remainder = total_size % n; 

    int start = 0;
    for (int i = 0; i < n; i++) {
        int current_part_size = part_size + (i < remainder ? 1 : 0); 
        parts.push_back(text.substr(start, current_part_size));
        start += current_part_size;
    }

    return parts;
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
    if (mode == "-c") {
        std::vector<CompressedBlockData*> blocks;
        std::vector<std::string> parts = split(input, thread_num);
        for (int i = 0; i < thread_num; i++) {
            blocks.push_back(new CompressedBlockData(i));
        }
        #pragma omp parallel for num_threads(thread_num) schedule(static)
        for (int i = 0; i < thread_num; i++) {
            for (char c : parts[i]) {
                blocks[i]->push_back(c);
            }
            blocks[i]->flush();
        }
        write(output, blocks);
        for (auto block : blocks) {
            delete block;
        }
    }
    else if (mode == "-x") {
        std::string compressed_data = input;
        std::vector<uint8_t> compressed_data_u8 = stringToU8(compressed_data);
        std::vector<DepressedBlockData*> decompressed_data = gzip_decompress(compressed_data_u8, thread_num);
        write(output, decompressed_data);
    }
    else {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file> <thread num> <-c/-x>" << std::endl;
        return 1;
    }
    return 0;
}