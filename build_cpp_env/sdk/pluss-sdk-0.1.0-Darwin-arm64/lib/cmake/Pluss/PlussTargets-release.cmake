#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Pluss::PlussTilsView" for configuration "Release"
set_property(TARGET Pluss::PlussTilsView APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Pluss::PlussTilsView PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX;OBJCXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libPlussTilsView.a"
  )

list(APPEND _cmake_import_check_targets Pluss::PlussTilsView )
list(APPEND _cmake_import_check_files_for_Pluss::PlussTilsView "${_IMPORT_PREFIX}/lib/libPlussTilsView.a" )

# Import target "Pluss::PlussTilsCore" for configuration "Release"
set_property(TARGET Pluss::PlussTilsCore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Pluss::PlussTilsCore PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libPlussTilsCore.a"
  )

list(APPEND _cmake_import_check_targets Pluss::PlussTilsCore )
list(APPEND _cmake_import_check_files_for_Pluss::PlussTilsCore "${_IMPORT_PREFIX}/lib/libPlussTilsCore.a" )

# Import target "Pluss::PlussPl" for configuration "Release"
set_property(TARGET Pluss::PlussPl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Pluss::PlussPl PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libPlussPl.a"
  )

list(APPEND _cmake_import_check_targets Pluss::PlussPl )
list(APPEND _cmake_import_check_files_for_Pluss::PlussPl "${_IMPORT_PREFIX}/lib/libPlussPl.a" )

# Import target "Pluss::PlussMic" for configuration "Release"
set_property(TARGET Pluss::PlussMic APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Pluss::PlussMic PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libPlussMic.a"
  )

list(APPEND _cmake_import_check_targets Pluss::PlussMic )
list(APPEND _cmake_import_check_files_for_Pluss::PlussMic "${_IMPORT_PREFIX}/lib/libPlussMic.a" )

# Import target "Pluss::pluss" for configuration "Release"
set_property(TARGET Pluss::pluss APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Pluss::pluss PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpluss.a"
  )

list(APPEND _cmake_import_check_targets Pluss::pluss )
list(APPEND _cmake_import_check_files_for_Pluss::pluss "${_IMPORT_PREFIX}/lib/libpluss.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
