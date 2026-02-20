cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)

add_library(PlussMic STATIC)

target_include_directories(PlussMic PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../PlussQit/include
    ${PROJECT_SOURCE_DIR}/../PlussQit/src
    ${PROJECT_SOURCE_DIR}/../ExLibs/fmt/include
)

target_compile_definitions(PlussMic PRIVATE FMT_HEADER_ONLY=1)

target_sources(PlussMic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(src/loader)
add_subdirectory(src/handle)
add_subdirectory(src/bundle)
add_subdirectory(src/table)

target_link_libraries(PlussMic PUBLIC
    PlussQit
    plussqit_codeh
    plussqit_codem
    plussqit_coden
)

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)

add_subdirectory(examples)
add_subdirectory(test)
