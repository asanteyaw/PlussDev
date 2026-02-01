file(GLOB NOTE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/note_*.cpp
)

target_sources(pluss PRIVATE ${NOTE_SOURCES})

add_subdirectory(examples)
add_subdirectory(test)