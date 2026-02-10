cmake_minimum_required(VERSION 3.28)
project(Plussible LANGUAGES CXX)

add_library(Plussible STATIC)

target_include_directories(Plussible PUBLIC
    ${PROJECT_SOURCE_DIR}/../include
)

target_sources(Plussible PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(csv)
add_subdirectory(handle)
add_subdirectory(record)
add_subdirectory(table)

target_link_libraries(Plussible PUBLIC PlussQit)

set_target_properties(Plussible PROPERTIES CXX_STANDARD 20)