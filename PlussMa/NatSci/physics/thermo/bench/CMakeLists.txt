add_executable(natsci_thermo_bench species_bench.cpp)
target_link_libraries(natsci_thermo_bench PRIVATE pluss)
target_include_directories(natsci_thermo_bench PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
