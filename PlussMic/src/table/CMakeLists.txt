cmake_minimum_required(VERSION 3.28)
project(PlussMic_table NONE)

target_sources(PlussMic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DataFrame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Atlas.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/ThreadIO.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/DataFrameIO.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/AtlasIO.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ops/DataFrameReshape.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/DataFramePlot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/detail/Plot3DConfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/DataFramePlotOps.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sql/DataFrameSql.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/AtlasPlot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/display/Display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DataFrame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Atlas.h
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/DataFramePlot.h
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/detail/Plot3DConfig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/plot/DataFramePlotOps.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sql/DataFrameSql.h
  ${CMAKE_CURRENT_SOURCE_DIR}/display/Display.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Table.h
)
