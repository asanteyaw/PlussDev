file(GLOB THREAD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_*.cpp
)

target_sources(pluss PRIVATE ${THREAD_SOURCES})

add_subdirectory(test)