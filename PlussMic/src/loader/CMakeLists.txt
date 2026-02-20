cmake_minimum_required(VERSION 3.0)
project(PlussMic_loader NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CsvLoader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CsvLoader.cpp
)
