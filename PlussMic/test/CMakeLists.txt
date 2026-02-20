cmake_minimum_required(VERSION 3.0)
project(PlussMic_tests NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_executable(plussmic_table_csv_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_csv_parser_tests.cpp
)

target_link_libraries(plussmic_table_csv_tests PRIVATE PlussMic)
target_compile_definitions(plussmic_table_csv_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
