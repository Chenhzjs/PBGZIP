cmake_minimum_required(VERSION 3.12)
project(ParallelBGZIP)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

add_executable(deflate deflate.cpp main.cpp serialize.cpp)

add_executable(generate generate.cpp)