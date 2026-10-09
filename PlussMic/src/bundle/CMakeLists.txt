# Bundle workspace models: Nexus (DataFrames) and Locus (Catalogs)

cmake_minimum_required(VERSION 3.28)
project(PlussMic_bundle NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Nexus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Nexus.h
  ${CMAKE_CURRENT_SOURCE_DIR}/LocusPull.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/WorkspaceStore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/WorkspaceStore.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Locus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Locus.h
)
