cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBZIP QUIET libzip)
endif()
project(plussma LANGUAGES CXX)

# PlussQit core numeric/time primitives are foundational for all higher layers
# (including PlussMic). Keep them always enabled at the top-level.
set(PLUSSQIT_BUILD_CODEM ON CACHE BOOL "Build CodeM (required by PlussMa libs)" FORCE)
set(PLUSSQIT_BUILD_CODEH ON CACHE BOOL "Build CodeH (required by PlussMa libs)" FORCE)

# Provide a top-level 'pluss' library so subdirectories can attach sources
add_library(pluss STATIC)
target_include_directories(pluss PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(pluss PROPERTIES CXX_STANDARD 20)

# Provide an alias so older CMakeLists that expect a `PlussQit` target still work
add_library(PlussQit ALIAS pluss)

# Define a simple EXCLUDE macro expected by some internal CMakeLists
# Removes items in ARGN from the list named by VAR
macro(EXCLUDE VAR)
    foreach(_itm IN LISTS ARGN)
        list(REMOVE_ITEM ${VAR} ${_itm})
    endforeach()
endmacro()

# Minimal append_filelist used by internal CMakeLists (stub for configure)
macro(append_filelist NAME VAR)
    set(_cur ${${NAME}})
    set(_add ${${VAR}})
    foreach(_i IN LISTS _add)
        list(APPEND _cur ${_i})
    endforeach()
    set(${NAME} ${_cur} PARENT_SCOPE)
endmacro()

# lowercase alias used in some files
macro(exclude) 
    _EXCLUDE_CALL(${ARGV0})
endmacro()

macro(_EXCLUDE_CALL VAR)
    foreach(_itm IN LISTS ARGN)
        list(REMOVE_ITEM ${VAR} ${_itm})
    endforeach()
endmacro()

# ==============================
# Global configuration
# ==============================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)

# ==============================
# Create libraries
# ==============================

add_subdirectory(PlussAI)
add_subdirectory(PlussQit)
if(NOT TARGET plussqit_coden)
    message(FATAL_ERROR "plussqit_coden (CodeN) must be built for PlussMa.")
endif()
if(NOT TARGET plussqit_codem)
    message(FATAL_ERROR "plussqit_codem (CodeM) must be built for PlussMa.")
endif()
if(NOT TARGET plussqit_codeh)
    message(FATAL_ERROR "plussqit_codeh (CodeH) must be built for PlussMa.")
endif()
target_link_libraries(pluss PUBLIC
    plussqit_codem
    plussqit_coden
    plussqit_codeh
)
add_subdirectory(PlussPl)
add_subdirectory(PlussMa)
add_subdirectory(PlussMic)

# Top-level aggregate target for downstream consumers that want the full stack.
# Use together with `#include <pluss/pluss.h>`.
add_library(pluss_sdk INTERFACE)
add_library(Pluss::SDK ALIAS pluss_sdk)

if(TARGET PlussQit)
    target_link_libraries(pluss_sdk INTERFACE PlussQit)
endif()
if(TARGET PlussPl)
    target_link_libraries(pluss_sdk INTERFACE PlussPl)
endif()
if(TARGET PlussMic)
    target_link_libraries(pluss_sdk INTERFACE PlussMic)
endif()
if(TARGET PlussTiq)
    target_link_libraries(pluss_sdk INTERFACE PlussTiq)
endif()
