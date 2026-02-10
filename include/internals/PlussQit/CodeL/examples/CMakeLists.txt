file(GLOB NOTE_EXAMPLES_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_ex.cpp
)

foreach(examples_src ${NOTE_EXAMPLES_SOURCES})
    get_filename_component(examples_name ${examples_src} NAME_WE)

    add_executable(${examples_name} ${examples_src})

    target_link_libraries(${examples_name} PRIVATE pluss)

    target_include_directories(${examples_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeM
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeL
        ${PROJECT_SOURCE_DIR}/include/internals/PlussQit/CodeN
    )

    # Optional but recommended: register example as a ctest test
    add_test(NAME ${examples_name} COMMAND ${examples_name})
    set_tests_properties(${examples_name} PROPERTIES WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

    message(STATUS "Added Note examples: ${examples_name}")
endforeach()