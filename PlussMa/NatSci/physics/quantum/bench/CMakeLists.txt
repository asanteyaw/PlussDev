add_executable(natsci_quantum_bench quantum_bench.cpp)
target_link_libraries(natsci_quantum_bench PRIVATE pluss)
target_include_directories(natsci_quantum_bench PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
