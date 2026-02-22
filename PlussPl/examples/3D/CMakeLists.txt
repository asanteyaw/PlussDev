cmake_minimum_required(VERSION 3.28)
project(PlussPl_examples_3D LANGUAGES CXX)

file(GLOB PLUSSPL_EXAMPLES_3D CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

foreach(src ${PLUSSPL_EXAMPLES_3D})
  get_filename_component(name ${src} NAME_WE)
  add_executable(plusspl_3d_${name} ${src})
  target_link_libraries(plusspl_3d_${name} PRIVATE PlussPl)
  target_compile_features(plusspl_3d_${name} PRIVATE cxx_std_20)
endforeach()
