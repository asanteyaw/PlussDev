add_executable(natsci_astro_bench orbit_bench.cpp)
target_link_libraries(natsci_astro_bench PRIVATE pluss)
target_include_directories(natsci_astro_bench PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
