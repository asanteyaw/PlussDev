cmake_minimum_required(VERSION 3.28)
project(PlussTica LANGUAGES CXX)

add_library(PlussTica INTERFACE)

target_include_directories(PlussTica INTERFACE
	${PROJECT_SOURCE_DIR}/../include
)

add_subdirectory(AI)
add_subdirectory(Eng)
add_subdirectory(Math)
add_subdirectory(NatSci)
add_subdirectory(Stats)

target_link_libraries(PlussTica INTERFACE PlussQit)

set_target_properties(PlussTica PROPERTIES CXX_STANDARD 20)