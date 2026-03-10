if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/CMakeLists.txt")
  add_subdirectory(core)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/forecasting/CMakeLists.txt")
  add_subdirectory(forecasting)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/arima/CMakeLists.txt")
  add_subdirectory(modules/arima)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/diagnostics/CMakeLists.txt")
  add_subdirectory(modules/diagnostics)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/var/CMakeLists.txt")
  add_subdirectory(modules/var)
endif()
