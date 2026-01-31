file(GLOB SERIES_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/series_*.cpp
)

target_sources(pluss PRIVATE ${SERIES_SOURCES})

add_subdirectory(examples)
add_subdirectory(test)