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

# Checks of the library's own helpers, built as a user would build them: only from the archive's
# headers and libraries. Each is run by cmake/ValidateDistribution.cmake.
set(PLUSS_SDK_CHECKS
    core_info:PlussTils/test/core_info_tests.cpp
    core_settings:PlussTils/test/core_settings_tests.cpp
    core_cache:PlussTils/test/core_cache_tests.cpp
    core_doc:PlussTils/test/core_doc_tests.cpp
    array_file:PlussQit/tests/array_file_tests.cpp
    examples:PlussMic/test/examples_tests.cpp
    umbrellas:examples/sdk_consumer/umbrellas.cpp)
foreach(_check IN LISTS PLUSS_SDK_CHECKS)
  string(REPLACE ":" ";" _parts "${_check}")
  list(GET _parts 0 _name)
  list(GET _parts 1 _source)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../${_source}")
    add_executable(pluss_sdk_check_${_name} "${CMAKE_CURRENT_SOURCE_DIR}/../../${_source}")
    target_link_libraries(pluss_sdk_check_${_name} PRIVATE Pluss::PlussII)
  endif()
endforeach()
