target_sources(pluss PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
)

add_subdirectory(csv)
add_subdirectory(handle)
add_subdirectory(registry)
add_subdirectory(table)