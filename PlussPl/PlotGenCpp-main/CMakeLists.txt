cmake_minimum_required(VERSION 3.10)

# Project name
project(PlotGenCpp VERSION 0.1 LANGUAGES CXX)

# Define C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)

# Find SFML
# Try to find SFML. Be permissive about versions/config packages so systems
# with different SFML packaging don't fail at configuration time.

# Fetch Simple SVG library but configure it manually to avoid GTest requirement
# include(FetchContent)
# FetchContent_Declare(
#     simple_svg
#     GIT_REPOSITORY https://github.com/adishavit/simple-svg.git
#     GIT_TAG master
# )
# # Just download the content but don't configure it yet
# FetchContent_GetProperties(simple_svg)
# if(NOT simple_svg_POPULATED)
#     FetchContent_Populate(simple_svg)
#     # Don't use FetchContent_MakeAvailable as we want to skip the original CMakeLists.txt
# endif()

# Simple SVG is header-only and the project already ships the header
# in `include/`, so just use the project include directory below.

# Add source files for the main program
# Source files
set(SOURCES src/plotgen.cpp)

# Public include dirs for library and examples
set(PROJECT_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/stb
)

include_directories(${PROJECT_INCLUDES})

# Create library
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDES})

# Add executable for examples
add_executable(PlotterExamples src/examples.cpp)
target_include_directories(PlotterExamples PRIVATE ${PROJECT_INCLUDES})


# Detect SFML installation by trying several fallbacks (pkg-config,
# CMake package, Homebrew default locations). We prefer Homebrew's
# `sfml@2` on macOS when available, but keep Linux behaviour unchanged.
set(SFML_TARGETS)

# Prepare preferred search paths. On macOS prefer Homebrew `sfml@2`.
set(SFML_PREFERRED_PATHS "")
if(APPLE)
    set(SFML_HOMEBREW_PREFIX "/opt/homebrew/opt/sfml@2")
    if(EXISTS "${SFML_HOMEBREW_PREFIX}")
        list(APPEND SFML_PREFERRED_PATHS
            "${SFML_HOMEBREW_PREFIX}/include"
            "${SFML_HOMEBREW_PREFIX}/lib"
            "${SFML_HOMEBREW_PREFIX}/lib/pkgconfig"
        )
        # Ensure pkg-config can see Homebrew sfml@2 .pc files first
        if(DEFINED ENV{PKG_CONFIG_PATH})
            set(ENV{PKG_CONFIG_PATH} "${SFML_HOMEBREW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        else()
            set(ENV{PKG_CONFIG_PATH} "${SFML_HOMEBREW_PREFIX}/lib/pkgconfig")
        endif()
        message(STATUS "Prioritizing Homebrew sfml@2 at: ${SFML_HOMEBREW_PREFIX}")
    endif()
endif()

# If Homebrew sfml@2 is present, prefer it explicitly (fast path).
if(APPLE AND EXISTS "${SFML_HOMEBREW_PREFIX}")
    if(EXISTS "${SFML_HOMEBREW_PREFIX}/include/SFML/Graphics.hpp"
       AND EXISTS "${SFML_HOMEBREW_PREFIX}/lib/libsfml-graphics.dylib")
        set(SFML_INCLUDE_ROOT "${SFML_HOMEBREW_PREFIX}/include")
        include_directories(${SFML_INCLUDE_ROOT})
        list(APPEND SFML_TARGETS
            "${SFML_HOMEBREW_PREFIX}/lib/libsfml-graphics.dylib"
            "${SFML_HOMEBREW_PREFIX}/lib/libsfml-window.dylib"
            "${SFML_HOMEBREW_PREFIX}/lib/libsfml-system.dylib"
        )
        message(STATUS "Using Homebrew sfml@2 explicitly: ${SFML_HOMEBREW_PREFIX}")
    endif()
endif()

# 1) Try pkg-config (common on Linux/macOS when SFML was built with pc files)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SFML_PKG QUIET sfml-graphics)
    if(SFML_PKG_FOUND)
        include_directories(${SFML_PKG_INCLUDE_DIRS})
        list(APPEND SFML_TARGETS ${SFML_PKG_LIBRARIES})
        message(STATUS "Found SFML via pkg-config: ${SFML_PKG_INCLUDE_DIRS}")
    endif()
endif()

# 2) Try CMake package targets (if available and well-behaved)
if(NOT SFML_TARGETS)
    if(TARGET SFML::Graphics)
        list(APPEND SFML_TARGETS SFML::Graphics SFML::Window SFML::System)
        message(STATUS "Found SFML imported targets (SFML::Graphics)")
    endif()
endif()

# 3) Filesystem search: prefer paths set above (Homebrew sfml@2 on macOS),
# then common system locations. This preserves the Linux behaviour while
# selecting Homebrew sfml@2 automatically on macOS when present.
if(NOT SFML_TARGETS)
    set(SFML_SEARCH_PATHS ${SFML_PREFERRED_PATHS} "/opt/homebrew/include" "/usr/local/include" "/usr/include")
    find_path(SFML_INCLUDE_DIR SFML/Graphics.hpp
        PATHS ${SFML_SEARCH_PATHS}
    )
    set(SFML_LIB_SEARCH_PATHS ${SFML_PREFERRED_PATHS} "/opt/homebrew/lib" "/usr/local/lib" "/usr/lib")
    find_library(SFML_GRAPHICS_LIB NAMES sfml-graphics PATHS ${SFML_LIB_SEARCH_PATHS})
    find_library(SFML_WINDOW_LIB NAMES sfml-window PATHS ${SFML_LIB_SEARCH_PATHS})
    find_library(SFML_SYSTEM_LIB NAMES sfml-system PATHS ${SFML_LIB_SEARCH_PATHS})
    if(SFML_INCLUDE_DIR AND SFML_GRAPHICS_LIB AND SFML_WINDOW_LIB AND SFML_SYSTEM_LIB)
        # If find_path returned a path that ends with "SFML/" we need the
        # parent include directory (so `#include <SFML/Graphics.hpp>` resolves
        # to ${SFML_INCLUDE_ROOT}/SFML/Graphics.hpp). Otherwise use the
        # directory returned by find_path directly.
        get_filename_component(SFML_BASENAME ${SFML_INCLUDE_DIR} NAME)
        if(SFML_BASENAME STREQUAL "SFML")
            get_filename_component(SFML_INCLUDE_PARENT ${SFML_INCLUDE_DIR} DIRECTORY)
            set(SFML_INCLUDE_ROOT ${SFML_INCLUDE_PARENT})
        else()
            set(SFML_INCLUDE_ROOT ${SFML_INCLUDE_DIR})
        endif()
        include_directories(${SFML_INCLUDE_ROOT})
        list(APPEND SFML_TARGETS ${SFML_GRAPHICS_LIB} ${SFML_WINDOW_LIB} ${SFML_SYSTEM_LIB})
        message(STATUS "Found SFML libraries in filesystem: include=${SFML_INCLUDE_ROOT} libs=${SFML_GRAPHICS_LIB};${SFML_WINDOW_LIB};${SFML_SYSTEM_LIB}")
    endif()
endif()

# Check for GTK3 and WEBKIT packages via pkg-config to enable optional
# HTML viewer features. If found, add their libraries to the link list.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK3 QUIET gtk+-3.0)
    pkg_check_modules(WEBKIT QUIET webkit2gtk-4.1)
    if(GTK3_FOUND AND WEBKIT_FOUND)
        add_compile_definitions(HAVE_GTK_WEBKIT)
        include_directories(${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
        list(APPEND SFML_TARGETS ${GTK3_LIBRARIES} ${WEBKIT_LIBRARIES})
        message(STATUS "GTK3 and WebKit found - HTMLViewer will be enabled")
    else()
        message(STATUS "GTK3 and/or WebKit not found - HTMLViewer will be disabled")
    endif()
else()
    message(STATUS "pkg-config not found - HTMLViewer will be disabled")
endif()

# Link SFML (or warn if not found). Using the computed SFML_TARGETS makes
# the CMake file resilient to different packaging styles.
if(SFML_TARGETS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SFML_TARGETS})
else()
    message(WARNING "SFML targets/libraries not found - linking may fail")
endif()

target_link_libraries(PlotterExamples PRIVATE ${PROJECT_NAME} ${SFML_TARGETS})

# Add compiler flags for optimization and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -O3
        -Wall
        # -Wextra
        # -Wpedantic
        # -Wno-unused-parameter
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /O2
        /W4
    )
endif()

# Installation (optional)
install(TARGETS ${PROJECT_NAME} PlotterExamples DESTINATION bin)
# Install font if it exists in the project root
if(EXISTS ${PROJECT_SOURCE_DIR}/arial.ttf)
    install(FILES arial.ttf DESTINATION bin)
endif()
