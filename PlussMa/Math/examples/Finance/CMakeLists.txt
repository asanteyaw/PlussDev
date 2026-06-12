function(plussma_add_math_finance_example source_file)
  get_filename_component(stem "${source_file}" NAME_WE)
  set(target_name "plussma_math_finance_${stem}")
  add_executable(${target_name} "${source_file}")
  target_compile_features(${target_name} PRIVATE cxx_std_20)
  target_link_libraries(${target_name} PRIVATE pluss plussqit_codem)
  set_target_properties(${target_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PlussMa/Math/examples/Finance"
  )
endfunction()

if(PLUSSMA_ENABLE_MATH
   AND PLUSSMA_MATH_ENABLE_FINANCE
   AND PLUSSMA_MATH_ENABLE_NUMERICAL
   AND PLUSSMA_MATH_NUMERICAL_ENABLE_OPTIMIZATION
   AND PLUSSMA_MATH_FINANCE_ENABLE_PRICING
   AND PLUSSMA_MATH_FINANCE_ENABLE_FILTERING
   AND PLUSSMA_MATH_FINANCE_ENABLE_STOCHASTIC_PROCESSES)
  plussma_add_math_finance_example(finance_filtering_demo.cpp)
  plussma_add_math_finance_example(finance_heston_filter_sp500_demo.cpp)
  plussma_add_math_finance_example(finance_heston_objective_mse_demo.cpp)
  plussma_add_math_finance_example(finance_objects_bates_mc_demo.cpp)
  plussma_add_math_finance_example(finance_objects_bs_analytic_demo.cpp)
  plussma_add_math_finance_example(finance_objects_bs_mc_demo.cpp)
  plussma_add_math_finance_example(finance_objects_heston_fft_mc_demo.cpp)
  plussma_add_math_finance_example(finance_objects_sabr_demo.cpp)
  plussma_add_math_finance_example(finance_pipeline_demo.cpp)
  plussma_add_math_finance_example(finance_roughvol_fft_demo.cpp)
  plussma_add_math_finance_example(finance_stochvol_schemes_demo.cpp)
  plussma_add_math_finance_example(pricing_models_demo.cpp)
  plussma_add_math_finance_example(roughvol_fft_demo.cpp)
endif()
