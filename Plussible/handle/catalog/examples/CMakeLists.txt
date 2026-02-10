# Catalog examples
add_executable(catalog_ex catalog_ex.cpp)
target_link_libraries(catalog_ex PRIVATE pluss)

# Advanced catalog examples with vectorized operations
add_executable(catalog_advanced_ex catalog_advanced_ex.cpp)
target_link_libraries(catalog_advanced_ex PRIVATE pluss)
