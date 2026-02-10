file(GLOB DATAFRAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/dataframe_*.cpp
)

# High-level (pandas-like) helpers live in dataframe.cpp
list(APPEND DATAFRAME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dataframe.cpp
)

target_sources(pluss PRIVATE ${DATAFRAME_SOURCES})

add_subdirectory(examples)
add_subdirectory(test)