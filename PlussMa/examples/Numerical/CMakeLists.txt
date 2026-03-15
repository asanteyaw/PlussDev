if(TARGET pluss AND TARGET plussqit_codem AND PLUSSMA_ENABLE_STATS AND PLUSSMA_STATS_ENABLE_DISTRIBUTIONS)
  add_executable(plussma_distributions_demo
    distributions_demo.cpp
  )
  target_compile_features(plussma_distributions_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_distributions_demo PRIVATE pluss plussqit_codem)
endif()

if(PLUSSMA_ENABLE_MATH AND PLUSSMA_MATH_ENABLE_NUMERICAL AND PLUSSMA_MATH_NUMERICAL_ENABLE_OPTIMIZATION)
  add_subdirectory(optimization)
endif()

if(PLUSSMA_ENABLE_MATH
   AND PLUSSMA_MATH_ENABLE_FINANCE
   AND PLUSSMA_MATH_FINANCE_ENABLE_PRICING
   AND PLUSSMA_MATH_FINANCE_ENABLE_CALIBRATION
   AND PLUSSMA_MATH_FINANCE_ENABLE_STOCHASTIC_PROCESSES)
  add_executable(plussma_finance_heston_sv_demo
    finance_heston_sv_demo.cpp
  )
  target_compile_features(plussma_finance_heston_sv_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_heston_sv_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_finance_stochvol_schemes_demo
    finance_stochvol_schemes_demo.cpp
  )
  target_compile_features(plussma_finance_stochvol_schemes_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_stochvol_schemes_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_finance_stochvol_objects_demo
    finance_stochvol_objects_demo.cpp
  )
  target_compile_features(plussma_finance_stochvol_objects_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_stochvol_objects_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_finance_bates_demo
    finance_bates_demo.cpp
  )
  target_compile_features(plussma_finance_bates_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_bates_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_finance_rates_objects_demo
    finance_rates_objects_demo.cpp
  )
  target_compile_features(plussma_finance_rates_objects_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_rates_objects_demo PRIVATE pluss plussqit_codem)
endif()

if(PLUSSMA_ENABLE_MATH
   AND PLUSSMA_MATH_ENABLE_FINANCE
   AND PLUSSMA_MATH_FINANCE_ENABLE_FILTERING)
  add_executable(plussma_finance_filtering_demo
    finance_filtering_demo.cpp
  )
  target_compile_features(plussma_finance_filtering_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_filtering_demo PRIVATE pluss plussqit_codem)
endif()

if(PLUSSMA_ENABLE_MATH
   AND PLUSSMA_MATH_ENABLE_FINANCE
   AND PLUSSMA_MATH_FINANCE_ENABLE_FILTERING
   AND PLUSSMA_MATH_ENABLE_NUMERICAL
   AND PLUSSMA_MATH_NUMERICAL_ENABLE_OPTIMIZATION)
  add_executable(plussma_finance_heston_filter_sp500_demo
    finance_heston_filter_sp500_demo.cpp
  )
  target_compile_features(plussma_finance_heston_filter_sp500_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_finance_heston_filter_sp500_demo PRIVATE pluss plussqit_codem)
endif()
