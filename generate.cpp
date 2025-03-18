#include <iostream>
#include <fstream>
#include <random>
#include <vector>

void generate_random_data(size_t mb, const std::string& filename) {
    size_t bytes = mb * 1024; 
    std::ofstream file(filename, std::ios::binary); 

    if (!file) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());          
    std::uniform_int_distribution<uint8_t> dist(0, 255); 

    std::vector<uint8_t> buffer(1024 * 1024); 
    size_t written = 0;

    while (written < bytes) {
        size_t chunk_size = std::min(buffer.size(), bytes - written);
        for (size_t i = 0; i < chunk_size; ++i) {
            buffer[i] = dist(gen);
        }
        file.write(reinterpret_cast<char*>(buffer.data()), chunk_size);
        written += chunk_size;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " < generated size (KB)>" << std::endl;
        return 1;
    }

    size_t mb = std::stoi(argv[1]);
    generate_random_data(mb, "random_data.bin");

    return 0;
}