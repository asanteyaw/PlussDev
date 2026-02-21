# Bundle model (Slot -> Registry -> Aisle)

cmake_minimum_required(VERSION 3.0)
project(PlussMic_bundle NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Slot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Registry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Aisle.cpp
)
