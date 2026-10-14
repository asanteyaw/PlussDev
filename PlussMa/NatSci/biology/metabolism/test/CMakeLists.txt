file(GLOB METABOLISM_TESTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

foreach(test_src ${METABOLISM_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE pluss)
    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
# The genome-scale model run takes minutes: label it so it can be excluded with `ctest -LE slow`.
if(TARGET genome_scale_test)
    set_tests_properties(genome_scale_test PROPERTIES LABELS "slow" TIMEOUT 7200)
endif()
