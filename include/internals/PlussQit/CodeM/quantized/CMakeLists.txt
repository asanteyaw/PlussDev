# Placeholder CMakeLists for optional 'quantized' module
cmake_minimum_required(VERSION 3.18)
if(NOT TARGET quantized)
  add_library(quantized INTERFACE)
endif()
