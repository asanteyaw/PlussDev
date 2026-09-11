if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core")
  add_subdirectory(core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lymph")
  add_subdirectory(lymph)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/classic")
  add_subdirectory(classic)
endif()
