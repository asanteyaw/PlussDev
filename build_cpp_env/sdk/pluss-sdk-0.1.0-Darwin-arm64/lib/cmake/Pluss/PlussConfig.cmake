
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was PlussConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

set(_pluss_targets_file "${CMAKE_CURRENT_LIST_DIR}/PlussTargets.cmake")
set(_pluss_targets_content "")
if(EXISTS "${_pluss_targets_file}")
  file(READ "${_pluss_targets_file}" _pluss_targets_content)
endif()

string(FIND "${_pluss_targets_content}" "Arrow::arrow" _pluss_needs_arrow)
string(FIND "${_pluss_targets_content}" "Parquet::parquet" _pluss_needs_parquet)
string(FIND "${_pluss_targets_content}" "CURL::libcurl" _pluss_needs_curl)

if(NOT _pluss_needs_arrow EQUAL -1)
  find_dependency(Arrow CONFIG QUIET)
  if(TARGET Arrow::arrow AND NOT TARGET Arrow::arrow_shared)
    add_library(Arrow::arrow_shared INTERFACE IMPORTED)
    set_target_properties(Arrow::arrow_shared PROPERTIES INTERFACE_LINK_LIBRARIES "Arrow::arrow")
  elseif(TARGET Arrow::arrow_shared AND NOT TARGET Arrow::arrow)
    add_library(Arrow::arrow INTERFACE IMPORTED)
    set_target_properties(Arrow::arrow PROPERTIES INTERFACE_LINK_LIBRARIES "Arrow::arrow_shared")
  endif()
endif()

if(NOT _pluss_needs_parquet EQUAL -1)
  find_dependency(Parquet CONFIG QUIET)
  if(TARGET Parquet::parquet AND NOT TARGET Parquet::parquet_shared)
    add_library(Parquet::parquet_shared INTERFACE IMPORTED)
    set_target_properties(Parquet::parquet_shared PROPERTIES INTERFACE_LINK_LIBRARIES "Parquet::parquet")
  elseif(TARGET Parquet::parquet_shared AND NOT TARGET Parquet::parquet)
    add_library(Parquet::parquet INTERFACE IMPORTED)
    set_target_properties(Parquet::parquet PROPERTIES INTERFACE_LINK_LIBRARIES "Parquet::parquet_shared")
  endif()
endif()

if(NOT _pluss_needs_curl EQUAL -1)
  find_dependency(CURL QUIET)
endif()

if(NOT _pluss_needs_arrow EQUAL -1 AND NOT TARGET Arrow::arrow AND NOT TARGET Arrow::arrow_shared)
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
      "Pluss SDK requires Arrow for PlussMic parquet support, but Arrow CMake targets were not found. "
      "Install Arrow/Parquet and add their prefix to CMAKE_PREFIX_PATH (for example your conda env).")
  return()
endif()

if(NOT _pluss_needs_parquet EQUAL -1 AND NOT TARGET Parquet::parquet AND NOT TARGET Parquet::parquet_shared)
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
      "Pluss SDK requires Parquet for PlussMic parquet support, but Parquet CMake targets were not found. "
      "Install Arrow/Parquet and add their prefix to CMAKE_PREFIX_PATH (for example your conda env).")
  return()
endif()

if(NOT _pluss_needs_curl EQUAL -1 AND NOT TARGET CURL::libcurl)
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
      "Pluss SDK requires libcurl for URL loading support, but CURL::libcurl was not found. "
      "Install libcurl (or build Pluss SDK with bundled curl enabled) and retry.")
  return()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/../PlussQit/PlussQitTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PlussTargets.cmake")

if(NOT TARGET Pluss::SDK AND TARGET Pluss::pluss)
  add_library(Pluss::SDK INTERFACE IMPORTED)
  # Default SDK surface for <pluss/pluss.h>: Qit + Mic + Pl.
  set(_pluss_sdk_link_libs "Pluss::pluss")
  if(TARGET Pluss::PlussMic)
    list(APPEND _pluss_sdk_link_libs "Pluss::PlussMic")
  endif()
  if(TARGET Pluss::PlussPl)
    list(APPEND _pluss_sdk_link_libs "Pluss::PlussPl")
  endif()
  set_target_properties(Pluss::SDK PROPERTIES
    INTERFACE_LINK_LIBRARIES "${_pluss_sdk_link_libs}"
  )
endif()

check_required_components(Pluss)
