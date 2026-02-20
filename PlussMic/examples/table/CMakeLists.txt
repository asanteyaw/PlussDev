cmake_minimum_required(VERSION 3.0)
project(PlussMic_examples_table NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_dataframe_initial_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_initial_demo.cpp
)

target_link_libraries(plussmic_dataframe_initial_demo PRIVATE PlussMic)
target_compile_definitions(plussmic_dataframe_initial_demo PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
