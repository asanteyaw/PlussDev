add_executable(natsci_fluids_bench fluids_bench.cpp)
target_link_libraries(natsci_fluids_bench PRIVATE pluss)
target_include_directories(natsci_fluids_bench PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
