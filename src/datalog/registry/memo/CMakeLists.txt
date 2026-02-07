file(GLOB RECORD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/record_*.cpp
)

target_sources(pluss PRIVATE ${RECORD_SOURCES})

add_subdirectory(test)