file(GLOB STREAM_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_*.cpp
)

target_sources(pluss PRIVATE ${STREAM_SOURCES})

add_subdirectory(test)