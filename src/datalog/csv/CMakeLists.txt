file(GLOB CSV_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(csv_parallel_obj OBJECT ${CSV_SOURCES})
target_link_libraries(csv_parallel_obj PUBLIC ${TORCH_LIBRARIES})
target_include_directories(csv_parallel_obj PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${TORCH_INCLUDE_DIRS}
)
