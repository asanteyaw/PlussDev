cmake_minimum_required(VERSION 3.28)
project(PlussTils LANGUAGES CXX)
include(GNUInstallDirs)

add_subdirectory(view)

add_library(PlussTilsCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/env/env.cpp
)
add_library(Pluss::TilsCore ALIAS PlussTilsCore)
target_include_directories(PlussTilsCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(PlussTilsCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_library(PlussTils INTERFACE)
add_library(Pluss::Tils ALIAS PlussTils)

target_link_libraries(PlussTils INTERFACE PlussTilsCore)
if(TARGET PlussTilsView)
    target_link_libraries(PlussTils INTERFACE PlussTilsView)
endif()
if(TARGET PlussQit)
    target_link_libraries(PlussTils INTERFACE PlussQit)
endif()

target_include_directories(PlussTils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS PlussTilsCore PlussTils
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
