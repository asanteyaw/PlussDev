cmake_minimum_required(VERSION 3.28)
project(PlussPl_tests LANGUAGES CXX)

include(CTest)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/2D/CMakeLists.txt")
  add_subdirectory(2D)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3D/CMakeLists.txt")
  add_subdirectory(3D)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/4D/CMakeLists.txt")
  add_subdirectory(4D)
endif()
