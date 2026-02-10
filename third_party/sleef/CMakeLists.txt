# Minimal placeholder for third_party/sleef used by configure
cmake_minimum_required(VERSION 3.18)
if(NOT TARGET sleef)
  add_library(sleef INTERFACE)
endif()
