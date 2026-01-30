file(GLOB FEED_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/feed_*.cpp
)

target_sources(pluss PRIVATE ${FEED_SOURCES})

add_subdirectory(test)