# Kindle model (Stream -> Catalog -> Archive)

cmake_minimum_required(VERSION 3.0)
project(PlussMic_kindle NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Catalog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Archive.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/display/Display.cpp
)
