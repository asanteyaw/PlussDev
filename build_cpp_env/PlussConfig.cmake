
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
