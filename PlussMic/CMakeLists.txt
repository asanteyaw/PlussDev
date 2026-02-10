cmake_minimum_required(VERSION 3.28)
project(PlussMic LANGUAGES CXX)

add_library(PlussMic INTERFACE)

target_include_directories(PlussMic INTERFACE
	${PROJECT_SOURCE_DIR}/../include
)

add_subdirectory(2D)
add_subdirectory(3D)
add_subdirectory(4D)

target_link_libraries(PlussMic INTERFACE PlussQit)

set_target_properties(PlussMic PROPERTIES CXX_STANDARD 20)