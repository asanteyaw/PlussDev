cmake_minimum_required(VERSION 3.28)
project(pluss_sdk_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss REQUIRED)

add_executable(pluss_sdk_consumer main.cpp)
target_link_libraries(pluss_sdk_consumer PRIVATE Pluss::PlussII)

# Round-trips a DataFrame through Excel and Parquet. A downloaded archive must
# support both with nothing else installed, so this fails if either is missing.
add_executable(pluss_sdk_native_io native_io.cpp)
target_link_libraries(pluss_sdk_native_io PRIVATE Pluss::PlussII)
