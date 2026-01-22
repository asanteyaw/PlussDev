file(GLOB BUNDLE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/bundle_*.cpp
)

target_sources(pluss PRIVATE ${BUNDLE_SOURCES})

add_subdirectory(test)