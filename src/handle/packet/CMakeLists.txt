file(GLOB PACKET_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/packet_*.cpp
)

target_sources(pluss PRIVATE ${PACKET_SOURCES})

add_subdirectory(test)