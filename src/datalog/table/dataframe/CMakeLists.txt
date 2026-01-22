file(GLOB DATAFRAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_*.cpp
)

target_sources(pluss PRIVATE ${DATAFRAME_SOURCES})

add_subdirectory(test)