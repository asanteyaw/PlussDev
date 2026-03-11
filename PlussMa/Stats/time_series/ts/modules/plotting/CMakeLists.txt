file(GLOB TS_PLOTTING_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${TS_PLOTTING_SOURCES})
if(EXISTS "${CMAKE_SOURCE_DIR}/PlussPl/include")
    target_include_directories(pluss PRIVATE "${CMAKE_SOURCE_DIR}/PlussPl/include")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()
