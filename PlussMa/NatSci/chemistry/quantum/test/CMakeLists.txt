file(GLOB CHEM_QUANTUM_TESTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

foreach(test_src ${CHEM_QUANTUM_TESTS})
    get_filename_component(test_file ${test_src} NAME_WE)

    # Global-unique, namespaced test target
    set(test_name "chem_quantum_${test_file}")

    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE pluss)

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()