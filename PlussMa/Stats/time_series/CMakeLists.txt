# time_series/CMakeLists.txt

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ts/CMakeLists.txt")
  add_subdirectory(ts)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/specialized/vol_models/CMakeLists.txt")
  add_subdirectory(specialized/vol_models)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
  add_subdirectory(examples)
endif()
