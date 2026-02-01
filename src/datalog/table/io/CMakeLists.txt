set(TABLE_IO_IMPL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/csv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/coordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/excel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parquet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rdbms.cpp
)

target_sources(pluss PRIVATE ${TABLE_IO_IMPL_SOURCES})

# If there are explicit tests under a `test/` subdirectory, build them as executables.
file(GLOB TABLE_IO_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
)

foreach(test_src ${TABLE_IO_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_link_libraries(${test_name} PRIVATE pluss)

    target_include_directories(${test_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

    message(STATUS "Added Table i/o test: ${test_name}")
endforeach()