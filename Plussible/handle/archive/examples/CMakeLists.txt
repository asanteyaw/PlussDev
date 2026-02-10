# Archive examples
add_executable(archive_ex archive_ex.cpp)
target_link_libraries(archive_ex PRIVATE pluss)

# Advanced archive examples with corpus management
add_executable(archive_advanced_ex archive_advanced_ex.cpp)
target_link_libraries(archive_advanced_ex PRIVATE pluss)
