cmake_minimum_required(VERSION 3.28)
project(PlussPl LANGUAGES CXX)

option(PLUSSPL_ENABLE_INTERACTIVE_SHOW "Enable graphical viewer launch from Figure::show()" OFF)
option(PLUSSPL_BUILD_EXAMPLES "Build PlussPl demos" ON)
option(PLUSSPL_BUILD_TESTS "Build PlussPl tests" ON)

add_library(PlussPl INTERFACE)

target_include_directories(PlussPl INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ExLibs/fmt/include
)

target_link_libraries(PlussPl INTERFACE PlussQit)

set_target_properties(PlussPl PROPERTIES CXX_STANDARD 20)
target_compile_features(PlussPl INTERFACE cxx_std_20)
target_compile_definitions(PlussPl INTERFACE FMT_HEADER_ONLY=1)

if(PLUSSPL_ENABLE_INTERACTIVE_SHOW)
    target_compile_definitions(PlussPl INTERFACE PLUSSPL_ENABLE_INTERACTIVE_SHOW=1)
    if(TARGET pluss)
        target_compile_definitions(pluss PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=1)
    endif()
else()
    target_compile_definitions(PlussPl INTERFACE PLUSSPL_ENABLE_INTERACTIVE_SHOW=0)
    if(TARGET pluss)
        target_compile_definitions(pluss PUBLIC PLUSSPL_ENABLE_INTERACTIVE_SHOW=0)
    endif()
endif()

if(PLUSSPL_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(PLUSSPL_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
