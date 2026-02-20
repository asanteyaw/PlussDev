cmake_minimum_required(VERSION 3.0)
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

target_link_libraries(plussmic_dataframe_initial_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_view_demo PRIVATE PlussMic)
target_link_libraries(plussmic_dataframe_describe_demo PRIVATE PlussMic)
target_link_libraries(plussmic_display_policy_demo PRIVATE PlussMic)

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
