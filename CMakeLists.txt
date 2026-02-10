cmake_minimum_required(VERSION 3.28)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/LocalConfig.cmake")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)
project(plussma LANGUAGES CXX)

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
# Create library 
# ==============================
# Core internals live under include/internals/PlussQit (CodeM/CodeN/CodeL)
add_subdirectory(include/internals/PlussQit/CodeM)
add_subdirectory(include/internals/PlussQit/CodeN)
add_subdirectory(include/internals/PlussQit/CodeL)

# Optional external PlussQit examples (kept separate)
if(EXISTS "${CMAKE_SOURCE_DIR}/PlussQit/CMakeLists.txt")
    add_subdirectory(PlussQit)
endif()

# Plussible, PlussTica and PlussMic are example/test modules that
# attach sources to the top-level 'pluss' target. Make them optional
# so a quick configure/build of the core `PlussQit` internals is fast.
option(BUILD_EXTRAS "Build example/test modules (Plussible/PlussTica/PlussMic)" OFF)
if(BUILD_EXTRAS)
    add_subdirectory(Plussible)
    add_subdirectory(PlussTica)
    add_subdirectory(PlussMic)
else()
    message(STATUS "Skipping Plussible/PlussTica/PlussMic (set -DBUILD_EXTRAS=ON to include)")
endif()
