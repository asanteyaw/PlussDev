cmake_minimum_required(VERSION 3.18)
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

# Export compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(plussma)

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

add_subdirectory(src/Series)
add_subdirectory(src/Chord)
add_subdirectory(src/Note)
add_subdirectory(src/TL)
add_subdirectory(src/factory)
add_subdirectory(src/error)
add_subdirectory(src/plotism/2D)
add_subdirectory(src/plotism/3D)
add_subdirectory(src/plotism/4D)
add_subdirectory(src/spaces)
add_subdirectory(src/table)
add_subdirectory(src/handle)
add_subdirectory(src/register)
add_subdirectory(test)

# === Link LibTorch ===
target_link_libraries(pluss PUBLIC ${TORCH_LIBRARIES})
target_include_directories(pluss PUBLIC ${PROJECT_SOURCE_DIR}/include)

# === Compiler Warnings & Optimizations ===
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-variable")

# === Tests ===
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/*.cpp")

# === Small tools ===
add_executable(jsonfmt src/json_formatter.cpp)
target_include_directories(jsonfmt PRIVATE ${PROJECT_SOURCE_DIR}/include)
set_target_properties(jsonfmt PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE pluss ${TORCH_LIBRARIES})
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    message(STATUS "✅ Test added: ${test_name}")
endforeach()