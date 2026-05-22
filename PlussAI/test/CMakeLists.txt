# AI module tests

find_package(Catch2 3 QUIET)

file(GLOB AI_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(FILTER AI_TEST_SOURCES EXCLUDE REGEX ".*/lymph_quick_test\\.cpp$")

foreach(test_src ${AI_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_link_libraries(${test_name} PRIVATE pluss)
    target_link_libraries(${test_name} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
    if(Catch2_FOUND)
        target_link_libraries(${test_name} PRIVATE Catch2::Catch2WithMain)
        target_compile_definitions(${test_name} PRIVATE PLUSS_AI_HAS_CATCH2=1)
    endif()
    target_compile_features(${test_name} PRIVATE cxx_std_20)

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/PlussMic/include
        ${PROJECT_SOURCE_DIR}/PlussMic/src
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
    if(DEFINED PLUSS_CUDNN_LIBRARY_DIR AND PLUSS_CUDNN_LIBRARY_DIR)
        set_property(TARGET ${test_name} APPEND PROPERTY BUILD_RPATH "${PLUSS_CUDNN_LIBRARY_DIR}")
    endif()
    if(DEFINED PLUSS_CUDNN_LIBRARY AND PLUSS_CUDNN_LIBRARY)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "PLUSS_CUDNN_LIBRARY=${PLUSS_CUDNN_LIBRARY}"
        )
    endif()

    message(STATUS "Added AI test: ${test_name}")
endforeach()
