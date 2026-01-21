cmake_minimum_required(VERSION 3.18)

set(CMAKE_C_COMPILER "/Users/yawasante/software/miniconda3/envs/cpp_env/bin/clang")
set(CMAKE_CXX_COMPILER "/Users/yawasante/software/miniconda3/envs/cpp_env/bin/clang++")

# Export compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(plussible)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Release mode by default
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

# Ensure ABI compatibility for LibTorch
add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)

# === Find LibTorch ===
find_package(Torch REQUIRED)

# === Include Paths ===
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/pluss
    ${TORCH_INCLUDE_DIRS}
)

# === Link Paths ===
link_directories(${TORCH_INSTALL_PREFIX}/lib)

# === Add modular sources ===
add_library(pluss STATIC)

add_subdirectory(value)
add_subdirectory(factory)
add_subdirectory(error)
add_subdirectory(plots/2D)
add_subdirectory(plots/3D)
add_subdirectory(plots/4D)
add_subdirectory(spaces/AI)
add_subdirectory(spaces/Econ)
add_subdirectory(spaces/Eng)
add_subdirectory(spaces/Math)
add_subdirectory(spaces/NatSci)
add_subdirectory(spaces/Stats)
add_subdirectory(datalog/table)
add_subdirectory(datalog/item)

# === Link LibTorch ===
target_link_libraries(pluss PUBLIC ${TORCH_LIBRARIES})
target_include_directories(pluss PUBLIC ${PROJECT_SOURCE_DIR}/include)

# === Compiler Warnings & Optimizations ===
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-variable")

# === Tests ===
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/*.cpp")

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE pluss ${TORCH_LIBRARIES})
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    message(STATUS "✅ Test added: ${test_name}")
endforeach()