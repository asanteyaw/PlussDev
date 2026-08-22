cmake_minimum_required(VERSION 3.28)
project(PlussMic_tests NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_executable(plussmic_table_csv_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_csv_parser_tests.cpp
)
add_executable(plussmic_kindle_catalog_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/kindle_catalog_tests.cpp
)
add_executable(plussmic_scribble_model_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/scribble_model_tests.cpp
)
add_executable(plussmic_table_dataframe_plot_api_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_dataframe_plot_api_tests.cpp
)
add_executable(plussmic_table_ts_accessor_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_ts_accessor_tests.cpp
)
add_executable(plussmic_table_axis_match_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_axis_match_tests.cpp
)
add_executable(plussmic_table_statistics_window_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_statistics_window_tests.cpp
)
add_executable(plussmic_table_flash_dataset_bridge_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/table_flash_dataset_bridge_tests.cpp
)
add_executable(plussmic_bi_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/pluss_bi_tests.cpp
)
add_executable(plussmic_table_gap_benchmark
  ${CMAKE_CURRENT_SOURCE_DIR}/table_gap_benchmark.cpp
)
add_executable(plussmic_scribble_gap_benchmark
  ${CMAKE_CURRENT_SOURCE_DIR}/scribble_gap_benchmark.cpp
)

target_link_libraries(plussmic_table_csv_tests PRIVATE PlussMic)
target_link_libraries(plussmic_kindle_catalog_tests PRIVATE PlussMic)
target_link_libraries(plussmic_scribble_model_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_dataframe_plot_api_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_ts_accessor_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_axis_match_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_statistics_window_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_flash_dataset_bridge_tests PRIVATE PlussMic)
target_link_libraries(plussmic_bi_tests PRIVATE PlussMic)
target_link_libraries(plussmic_table_gap_benchmark PRIVATE PlussMic)
target_link_libraries(plussmic_scribble_gap_benchmark PRIVATE PlussMic)
target_compile_definitions(plussmic_table_csv_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_kindle_catalog_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_scribble_model_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_table_ts_accessor_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_table_axis_match_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_table_statistics_window_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_table_flash_dataset_bridge_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_bi_tests PRIVATE
  FMT_HEADER_ONLY=1
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)

add_test(NAME plussmic_table_csv COMMAND plussmic_table_csv_tests)
add_test(NAME plussmic_kindle_catalog COMMAND plussmic_kindle_catalog_tests)
add_test(NAME plussmic_scribble_model COMMAND plussmic_scribble_model_tests)
add_test(NAME plussmic_table_dataframe_plot_api COMMAND plussmic_table_dataframe_plot_api_tests)
add_test(NAME plussmic_table_ts_accessor COMMAND plussmic_table_ts_accessor_tests)
add_test(NAME plussmic_table_axis_match COMMAND plussmic_table_axis_match_tests)
add_test(NAME plussmic_table_statistics_window COMMAND plussmic_table_statistics_window_tests)
add_test(NAME plussmic_table_flash_dataset_bridge COMMAND plussmic_table_flash_dataset_bridge_tests)
add_test(NAME plussmic_bi COMMAND plussmic_bi_tests)
