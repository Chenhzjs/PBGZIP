cmake_minimum_required(VERSION 3.12)
project(ParallelBGZIP)

set(CMAKE_CXX_STANDARD 17)
# if is macOS
if(APPLE)
    set(CMAKE_C_COMPILER gcc-14)
    set(CMAKE_CXX_COMPILER g++-14)
endif()


include_directories(include third_party/include)
include_directories(/usr/local/include)

find_package(TBB REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(deflate deflate.cpp main.cpp serialize.cpp)

target_compile_options(deflate PRIVATE -fopenmp)

target_link_libraries(deflate TBB::tbb gomp ZLIB::ZLIB)

add_executable(generate generate.cpp)