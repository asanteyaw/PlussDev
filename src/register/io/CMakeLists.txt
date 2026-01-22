file(GLOB REGISTER_IO_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

foreach(test_src ${REGISTER_IO_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_link_libraries(${test_name} PRIVATE pluss)

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    # Optional but recommended
    add_test(NAME ${test_name} COMMAND ${test_name})

    message(STATUS "Added Register i/o test: ${test_name}")
endforeach()