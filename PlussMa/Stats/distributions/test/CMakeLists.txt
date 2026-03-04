file(GLOB DISTRIBUTIONS_TESTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

foreach(test_src ${DISTRIBUTIONS_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE pluss plussqit_codem)

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

find_package(Catch2 3 QUIET)
if(TARGET Catch2::Catch2WithMain)
    add_executable(distr_objects_catch2
        distr_objects_catch2.cpp
    )
    target_link_libraries(distr_objects_catch2 PRIVATE pluss plussqit_codem Catch2::Catch2WithMain)
    target_compile_features(distr_objects_catch2 PRIVATE cxx_std_20)
    add_test(NAME distr_objects_catch2 COMMAND distr_objects_catch2)
endif()
