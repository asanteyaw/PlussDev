# Table-agnostic relational core shared by table, kindle and bundle:
# link/plan vocabulary, join-spec resolution, cached path search, key matching.

cmake_minimum_required(VERSION 3.28)
project(PlussMic_relational NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Links.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Links.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Codec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Codec.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Pull.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Pull.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Match.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Match.h
)
