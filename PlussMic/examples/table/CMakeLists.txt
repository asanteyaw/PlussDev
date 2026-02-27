cmake_minimum_required(VERSION 3.28)
project(PlussMic_examples_table NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_dataframe_initial_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_initial_demo.cpp
)
add_executable(plussmic_dataframe_view_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_view_demo.cpp
)
add_executable(plussmic_dataframe_describe_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_describe_demo.cpp
)
add_executable(plussmic_display_policy_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/display_policy_demo.cpp
)
add_executable(plussmic_df_ex_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/df_ex_demo.cpp
)
add_executable(plussmic_dataframe_versatile_methods_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_versatile_methods_demo.cpp
)
add_executable(plussmic_us_accidents_eda_verbs_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/us_accidents_eda_verbs_demo.cpp
)
add_executable(plussmic_eda_us_accidents_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/eda_us_accidents.cpp
)
add_executable(plussmic_dataframe_plot_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_plot_demo.cpp
)
add_executable(plussmic_dataframe_thread_values_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_thread_values_demo.cpp
)
add_executable(plussmic_loader_saver_parity_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/loader_saver_parity_demo.cpp
)

target_link_libraries(plussmic_dataframe_initial_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_view_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_describe_demo PRIVATE PlussMic)
target_link_libraries(plussmic_display_policy_demo PRIVATE PlussMic)
target_link_libraries(plussmic_df_ex_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_versatile_methods_demo PRIVATE PlussMic)
target_link_libraries(plussmic_us_accidents_eda_verbs_demo PRIVATE PlussMic)
target_link_libraries(plussmic_eda_us_accidents_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_plot_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_thread_values_demo PRIVATE PlussMic)
target_link_libraries(plussmic_loader_saver_parity_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_dataframe_initial_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_view_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_describe_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_display_policy_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_df_ex_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_versatile_methods_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_us_accidents_eda_verbs_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_eda_us_accidents_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_plot_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_dataframe_thread_values_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_loader_saver_parity_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
