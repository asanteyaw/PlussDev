# Bundle workspace model (Nexus and future workbench surfaces)

cmake_minimum_required(VERSION 3.28)
project(PlussMic_bundle NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Nexus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Nexus.h
)
