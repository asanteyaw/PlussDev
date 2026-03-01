file(GLOB DISTRIBUTIONS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${DISTRIBUTIONS_SOURCES})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()
