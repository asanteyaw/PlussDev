cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(plussma LANGUAGES CXX)

include(CTest)

# ==============================
# Global configuration
# ==============================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)

# ==============================
# Create library 
# ==============================
add_library(pluss STATIC)

# ==============================
# Dependencies
# ==============================
find_package(Torch REQUIRED)
find_package(OpenSSL REQUIRED)

# Show where OpenSSL was found and add include dir as a fallback
message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
if(DEFINED OPENSSL_INCLUDE_DIR)
    target_include_directories(pluss PUBLIC ${OPENSSL_INCLUDE_DIR})
endif()

target_link_libraries(pluss
    PUBLIC  ${TORCH_LIBRARIES}
    PRIVATE OpenSSL::Crypto
)

# ==============================
# Include paths
# ==============================
target_include_directories(pluss
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/pluss
        ${TORCH_INCLUDE_DIRS}
)

# ==============================
# Subdirectories (add sources)
# ==============================
add_subdirectory(src/Series)
add_subdirectory(src/Chord)
add_subdirectory(src/Note)
add_subdirectory(src/TL)
add_subdirectory(src/error)
add_subdirectory(src/plotism)
add_subdirectory(src/spaces)
add_subdirectory(src/datalog)
add_subdirectory(test)
add_subdirectory(examples)

# ==============================
# Compiler flags
# ==============================
target_compile_options(pluss PRIVATE
    -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-variable
)
