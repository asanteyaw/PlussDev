# Scribble model (Slot -> Registry -> Axel)

cmake_minimum_required(VERSION 3.28)
project(PlussMic_scribble NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Slot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Registry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Axel.cpp
)
