file(GLOB ODE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(pluss PRIVATE ${ODE_SOURCES})
target_include_directories(pluss PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/PlussQit/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/PlussQit/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/PlussMa/Math/Numerical/ode>
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()
