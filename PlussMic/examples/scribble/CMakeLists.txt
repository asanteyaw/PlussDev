cmake_minimum_required(VERSION 3.28)
project(PlussMic_examples_scribble NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_scribble_model_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/scribble_model_demo.cpp
)

target_link_libraries(plussmic_scribble_model_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_scribble_model_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)

add_executable(plussmic_scribble_accessor_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/scribble_accessor_demo.cpp
)

target_link_libraries(plussmic_scribble_accessor_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_scribble_accessor_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
