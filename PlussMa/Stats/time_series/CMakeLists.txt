# time_series/CMakeLists.txt

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arima/CMakeLists.txt")
  add_subdirectory(arima)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vol_models/CMakeLists.txt")
  add_subdirectory(vol_models)
endif()
