# Traditional implementation files
file(GLOB CHORD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/chord_*.cpp
)

target_sources(pluss PRIVATE ${CHORD_SOURCES})

add_subdirectory(test)