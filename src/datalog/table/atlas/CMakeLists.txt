file(GLOB ATLAS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${ATLAS_SOURCES})

add_subdirectory(test)
add_subdirectory(examples)