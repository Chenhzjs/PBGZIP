#include "deflate.h"
#include <iostream>
#include <fstream>
#include <filesystem>


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
    std::ofstream out_file(out_filename, std::ios::binary);
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
    // if (a.size() != b.size()) {
    //     std::cerr << "Different sizes: " << a.size() << " vs " << b.size() << std::endl;
    //     return false;
    // }
    // std::cout << b[b.size() - 1] << std::endl;
    // for (size_t i = 0; i < a.size(); i ++) {
    //     if (a[i] != b[i]) {
    //         std::cerr << "Mismatch at position " << i << ": " << a[i] << " vs " << b[i] << std::endl;
    //         return false;
    //     }
    // }
    std::ifstream file_a(a, std::ios::binary);
    std::ifstream file_b(b, std::ios::binary);
    for (;;) {
        char byte_a, byte_b;
        file_a.read(&byte_a, sizeof(byte_a));
        file_b.read(&byte_b, sizeof(byte_b));
        if (file_a.eof() && file_b.eof()) {
            break;  
        }
        if (file_a.eof() || file_b.eof()) {
            std::cerr << "Files have different sizes." << std::endl;
            // return false;
        }
        if (byte_a != byte_b) {
            std::cerr << "Mismatch at position " << file_a.tellg() << ": " << byte_a << " vs " << byte_b << std::endl;
            // return false;
        }
    }
    return true;
}

std::vector<int> get_start_index(const std::string &file_name, int n) {
    std::vector<int> start_indexes;

    int64_t file_size = std::filesystem::file_size(file_name);
    int64_t part_size = file_size / n;
    int64_t remainder = file_size % n;
    int start_index = 0;
    for (int i = 0; i < n; i++) {
        start_indexes.push_back(start_index);
        start_index += part_size;
        if (remainder > 0) {
            start_index ++;
            remainder --;
        }
    }
    return start_indexes;
}

int main(int argc, char* argv[]) {
    // std::string text = "abcabcabcabcabcabcabcabcabc";
    // std::string text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaababfbadbfdbadfsadfdsafadsfdsafdasfdsfewgjfkijvoasdjoas";
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file> <thread num> <-c/-x>" << std::endl;
        return 1;
    }
    // std::string input = read(argv[1]);
    std::string output = argv[2];
    int thread_num = std::stoi(argv[3]);
    std::string mode = argv[4];
    if (mode == "-c") {
        std::string input_name = argv[1];
        std::vector<CompressedBlockData*> blocks;
        std::vector<int> start_indexes = get_start_index(input_name, thread_num);
        std::vector<int> max_counts;
        for (int i = 0; i < thread_num; i ++) {
            if (i == thread_num - 1) {
                max_counts.push_back(std::filesystem::file_size(input_name) - start_indexes[i]);
            } else {
                max_counts.push_back(start_indexes[i + 1] - start_indexes[i]);
            }
        }
        for (int i = 0; i < thread_num; i ++) {
            blocks.push_back(new CompressedBlockData(i, input_name, start_indexes[i], max_counts[i]));
        }
        #pragma omp parallel for num_threads(thread_num) schedule(static)
        for (int i = 0; i < thread_num; i ++) {
            blocks[i]->run();
        }
        write(output, blocks);
        for (auto block : blocks) {
            delete block;
        }
    }
    else if (mode == "-x") {
        std::string input_name = argv[1];
        std::vector<DepressedBlockData*> decompressed_data = gzip_decompress(input_name, thread_num);
        write(output, decompressed_data);
        // diff(output, "data/random_data.txt");
    }
    else {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file> <thread num> <-c/-x>" << std::endl;
        return 1;
    }
    return 0;
}