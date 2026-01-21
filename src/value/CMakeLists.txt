# target_sources(pluss PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/value.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/value_options.cpp
# )

file(GLOB VALUE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${VALUE_SOURCES})