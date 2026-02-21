cmake_minimum_required(VERSION 3.0)
project(PlussMic_examples_signal NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_signal_model_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/signal_model_demo.cpp
)

target_link_libraries(plussmic_signal_model_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_signal_model_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)

