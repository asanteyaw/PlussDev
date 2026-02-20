cmake_minimum_required(VERSION 3.0)
project(PlussMic_table NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DataFrame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Atlas.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/display/Display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DataFrame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Atlas.h
  ${CMAKE_CURRENT_SOURCE_DIR}/display/Display.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Table.h
)
