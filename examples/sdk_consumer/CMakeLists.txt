cmake_minimum_required(VERSION 3.28)
project(pluss_sdk_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(pluss_sdk_consumer main.cpp)
target_link_libraries(pluss_sdk_consumer PRIVATE Pluss::PlussII)
