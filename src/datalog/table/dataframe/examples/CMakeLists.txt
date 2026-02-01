file(GLOB DATAFRAME_EXAMPLES_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_ex.cpp
)

foreach(EXAMPLES_src ${DATAFRAME_EXAMPLES_SOURCES})
    get_filename_component(EXAMPLES_name ${EXAMPLES_src} NAME_WE)

    add_executable(${EXAMPLES_name} ${EXAMPLES_src})

    target_link_libraries(${EXAMPLES_name} PRIVATE pluss)

    target_include_directories(${EXAMPLES_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    # Optional but recommended
    add_test(NAME ${EXAMPLES_name} COMMAND ${EXAMPLES_name})

    message(STATUS "Added Dataframe examples: ${EXAMPLES_name}")
endforeach()