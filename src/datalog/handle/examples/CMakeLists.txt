# Handle Examples

# handle_csv_example - Demonstrates CSV reading and advanced Handle features
add_executable(handle_csv_example 
    handle_csv_example.cpp
)

target_link_libraries(handle_csv_example PRIVATE
    pluss_core
    "${TORCH_LIBRARIES}"
)

target_include_directories(handle_csv_example PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${TORCH_INCLUDE_DIRS}
)

set_target_properties(handle_csv_example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
