cmake_minimum_required(VERSION 3.28)
project(plussii_code_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss REQUIRED)

add_executable(plussii_code_smoke main.cpp)
target_link_libraries(plussii_code_smoke PRIVATE Pluss::PlussII)
