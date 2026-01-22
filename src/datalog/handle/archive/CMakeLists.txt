file(GLOB ARCHIVE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/archive_*.cpp
)

target_sources(pluss PRIVATE ${ARCHIVE_SOURCES})

add_subdirectory(test)