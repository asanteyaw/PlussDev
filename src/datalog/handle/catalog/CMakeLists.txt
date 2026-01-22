file(GLOB CATALOG_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog_*.cpp
)

target_sources(pluss PRIVATE ${CATALOG_SOURCES})

add_subdirectory(test)