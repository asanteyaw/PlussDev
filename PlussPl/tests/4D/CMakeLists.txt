cmake_minimum_required(VERSION 3.28)
project(PlussPl_tests_4D LANGUAGES CXX)

file(GLOB PLUSSPL_TESTS_4D CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

foreach(src ${PLUSSPL_TESTS_4D})
  get_filename_component(name ${src} NAME_WE)
  add_executable(plusspl_test_4d_${name} ${src})
  target_link_libraries(plusspl_test_4d_${name} PRIVATE PlussPl)
  target_compile_features(plusspl_test_4d_${name} PRIVATE cxx_std_20)
  if(BUILD_TESTING)
    add_test(NAME plusspl_test_4d_${name} COMMAND plusspl_test_4d_${name})
  endif()
endforeach()
