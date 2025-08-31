cmake_minimum_required(VERSION 3.10)
project(LowpassFilter)

set(CMAKE_CXX_STANDARD 17)
find_package(PNG REQUIRED)

add_executable(lowpass src/main.cpp)
target_link_libraries(lowpass PNG::PNG)
