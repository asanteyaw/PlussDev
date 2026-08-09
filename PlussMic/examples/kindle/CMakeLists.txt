cmake_minimum_required(VERSION 3.28)
project(PlussMic_examples_kindle NONE)

get_filename_component(PLUSSMIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_executable(plussmic_kindle_model_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/kindle_model_demo.cpp
)
add_executable(plussmic_kindle_catalog_plot_demo
  ${CMAKE_CURRENT_SOURCE_DIR}/catalog_plot_demo.cpp
)

target_link_libraries(plussmic_kindle_model_demo PRIVATE PlussMic)
target_link_libraries(plussmic_kindle_catalog_plot_demo PRIVATE PlussMic)

target_compile_definitions(plussmic_kindle_model_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
target_compile_definitions(plussmic_kindle_catalog_plot_demo PRIVATE
  PLUSSMIC_SOURCE_DIR=\"${PLUSSMIC_ROOT_DIR}\"
)
