# Placeholder CMakeLists for optional 'nnapi' module
cmake_minimum_required(VERSION 3.18)
if(NOT TARGET nnapi)
  add_library(nnapi INTERFACE)
endif()
