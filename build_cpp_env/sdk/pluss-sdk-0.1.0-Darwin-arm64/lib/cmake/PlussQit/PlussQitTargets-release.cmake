#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pluss::plussqit_coden" for configuration "Release"
set_property(TARGET pluss::plussqit_coden APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pluss::plussqit_coden PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libplussqit_coden.a"
  )

list(APPEND _cmake_import_check_targets pluss::plussqit_coden )
list(APPEND _cmake_import_check_files_for_pluss::plussqit_coden "${_IMPORT_PREFIX}/lib/libplussqit_coden.a" )

# Import target "pluss::plussqit_codem" for configuration "Release"
set_property(TARGET pluss::plussqit_codem APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pluss::plussqit_codem PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX;OBJCXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libplussqit_codem.a"
  )

list(APPEND _cmake_import_check_targets pluss::plussqit_codem )
list(APPEND _cmake_import_check_files_for_pluss::plussqit_codem "${_IMPORT_PREFIX}/lib/libplussqit_codem.a" )

# Import target "pluss::plussqit_codeh" for configuration "Release"
set_property(TARGET pluss::plussqit_codeh APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pluss::plussqit_codeh PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libplussqit_codeh.a"
  )

list(APPEND _cmake_import_check_targets pluss::plussqit_codeh )
list(APPEND _cmake_import_check_files_for_pluss::plussqit_codeh "${_IMPORT_PREFIX}/lib/libplussqit_codeh.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
