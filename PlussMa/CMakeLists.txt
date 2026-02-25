cmake_minimum_required(VERSION 3.28)
project(PlussTiq LANGUAGES CXX)

add_library(PlussTiq INTERFACE)

target_include_directories(PlussTiq INTERFACE
	${PROJECT_SOURCE_DIR}/../include
)

target_include_directories(pluss PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(Eng)
add_subdirectory(Math)
add_subdirectory(NatSci)
add_subdirectory(Stats)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
	add_subdirectory(examples)
endif()

target_link_libraries(PlussTiq INTERFACE PlussQit)

set_target_properties(PlussTiq PROPERTIES CXX_STANDARD 20)
