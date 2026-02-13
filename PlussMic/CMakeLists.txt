cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)

add_library(PlussMic STATIC)

target_include_directories(PlussMic PUBLIC
    ${PROJECT_SOURCE_DIR}/../include
)

target_sources(PlussMic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(loader)
add_subdirectory(handle)
add_subdirectory(record)
add_subdirectory(table)

target_link_libraries(PlussMic PUBLIC PlussQit)

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)