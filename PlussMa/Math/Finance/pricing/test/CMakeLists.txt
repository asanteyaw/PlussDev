file(GLOB FIN_PRICING_TESTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*_catch2.cpp
)

find_package(Catch2 3 QUIET)

foreach(test_src ${FIN_PRICING_TESTS})
    get_filename_component(test_name ${test_src} NAME_WE)
    if(test_name MATCHES "_catch2$")
        if(TARGET Catch2::Catch2WithMain)
            add_executable(${test_name} ${test_src})
            target_link_libraries(${test_name} PRIVATE pluss plussqit_codem Catch2::Catch2WithMain)
            target_compile_features(${test_name} PRIVATE cxx_std_20)
            add_test(NAME ${test_name} COMMAND ${test_name})
        else()
            message(STATUS "Skipping ${test_name}: Catch2::Catch2WithMain not found")
        endif()
    else()
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE pluss plussqit_codem)
        target_compile_features(${test_name} PRIVATE cxx_std_20)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endif()
endforeach()
