# Biology: population

file(GLOB POPULATION_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${POPULATION_SOURCES})

target_include_directories(pluss PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()