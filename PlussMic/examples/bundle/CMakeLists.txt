cmake_minimum_required(VERSION 3.0)
project(PlussMic_examples_bundle NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_bundle_model_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/bundle_model_demo.cpp
)

target_link_libraries(plussmic_bundle_model_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_bundle_model_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)

