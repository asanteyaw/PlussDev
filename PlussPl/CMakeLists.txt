cmake_minimum_required(VERSION 3.28)
project(PlussPl LANGUAGES CXX)

add_library(PlussPl INTERFACE)

target_include_directories(PlussPl INTERFACE
		${PROJECT_SOURCE_DIR}/../include
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/2D/CMakeLists.txt")
	add_subdirectory(2D)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3D/CMakeLists.txt")
	add_subdirectory(3D)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/4D/CMakeLists.txt")
	add_subdirectory(4D)
endif()

target_link_libraries(PlussPl INTERFACE PlussQit)

set_target_properties(PlussPl PROPERTIES CXX_STANDARD 20)