cmake_minimum_required(VERSION 3.28)
project(plussmic_manual_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(plussmic_manual_examples plussmic_manual_examples.cpp)
target_link_libraries(plussmic_manual_examples PRIVATE Pluss::SDK)
