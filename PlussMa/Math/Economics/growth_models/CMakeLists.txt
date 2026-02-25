file(GLOB GROWTH_MODELS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${GROWTH_MODELS_SOURCES})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()