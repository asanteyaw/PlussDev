cmake_minimum_required(VERSION 3.28)
project(PlussPl LANGUAGES CXX)

add_library(PlussPl STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/theme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/lens_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/pane.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/core/studio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engines/spatex/spatex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plotism/render/export_frontend.cpp
)

add_library(Pluss::PlussPl ALIAS PlussPl)

target_include_directories(PlussPl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PlussQit/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(PlussPl PUBLIC
    PlussView
    PlussQit
    plussqit_codem
)

set_target_properties(PlussPl PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

install(TARGETS PlussPl
    EXPORT PlussTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN ".DS_Store" EXCLUDE
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()
