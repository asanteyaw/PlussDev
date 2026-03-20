if(TARGET pluss AND TARGET plussqit_codem)
  add_executable(plussma_ts_partition_raw_demo
    ts_partition_raw_demo.cpp
  )
  target_compile_features(plussma_ts_partition_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_partition_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_lag_features_raw_demo
    ts_lag_features_raw_demo.cpp
  )
  target_compile_features(plussma_ts_lag_features_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_lag_features_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_direct_forecaster_raw_demo
    ts_direct_forecaster_raw_demo.cpp
  )
  target_compile_features(plussma_ts_direct_forecaster_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_direct_forecaster_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_direct_cv_raw_demo
    ts_direct_cv_raw_demo.cpp
  )
  target_compile_features(plussma_ts_direct_cv_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_direct_cv_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_diagnostics_raw_demo
    ts_diagnostics_raw_demo.cpp
  )
  target_compile_features(plussma_ts_diagnostics_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_diagnostics_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_plotting_raw_demo
    ts_plotting_raw_demo.cpp
  )
  target_compile_features(plussma_ts_plotting_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_plotting_raw_demo PRIVATE pluss plussqit_codem)
  if(EXISTS "${CMAKE_SOURCE_DIR}/PlussPl/include")
    target_include_directories(plussma_ts_plotting_raw_demo PRIVATE "${CMAKE_SOURCE_DIR}/PlussPl/include")
  endif()
  if(TARGET PlussPl)
    target_link_libraries(plussma_ts_plotting_raw_demo PRIVATE PlussPl)
  endif()

  add_executable(plussma_ts_garch11_raw_demo
    ts_garch11_raw_demo.cpp
  )
  target_compile_features(plussma_ts_garch11_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_garch11_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_garchpq_raw_demo
    ts_garchpq_raw_demo.cpp
  )
  target_compile_features(plussma_ts_garchpq_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_garchpq_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_egarch_gjr_raw_demo
    ts_egarch_gjr_raw_demo.cpp
  )
  target_compile_features(plussma_ts_egarch_gjr_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_egarch_gjr_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_mv_vol_raw_demo
    ts_mv_vol_raw_demo.cpp
  )
  target_compile_features(plussma_ts_mv_vol_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_mv_vol_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_arima_sarima_raw_demo
    ts_arima_sarima_raw_demo.cpp
  )
  target_compile_features(plussma_ts_arima_sarima_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_arima_sarima_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_arimax_raw_demo
    ts_arimax_raw_demo.cpp
  )
  target_compile_features(plussma_ts_arimax_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_arimax_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_var_raw_demo
    ts_var_raw_demo.cpp
  )
  target_compile_features(plussma_ts_var_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_var_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_vecm_raw_demo
    ts_vecm_raw_demo.cpp
  )
  target_compile_features(plussma_ts_vecm_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_vecm_raw_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_ts_ssm_raw_demo
    ts_ssm_raw_demo.cpp
  )
  target_compile_features(plussma_ts_ssm_raw_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_ts_ssm_raw_demo PRIVATE pluss plussqit_codem)
endif()
