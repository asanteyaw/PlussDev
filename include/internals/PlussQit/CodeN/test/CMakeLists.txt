file(GLOB TIMELINE_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

foreach(test_src ${TIMELINE_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_link_libraries(${test_name} PRIVATE pluss)

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeM
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeL
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeN
    )

    # Optional but recommended
    add_test(NAME ${test_name} COMMAND ${test_name})

    message(STATUS "Added Timeline test: ${test_name}")
endforeach()