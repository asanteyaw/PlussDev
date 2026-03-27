file(GLOB HYP_TEST_ROOT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${HYP_TEST_ROOT_SOURCES})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/parametric/CMakeLists.txt")
    add_subdirectory(parametric)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nonparametric/CMakeLists.txt")
    add_subdirectory(nonparametric)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gof/CMakeLists.txt")
    add_subdirectory(gof)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()
