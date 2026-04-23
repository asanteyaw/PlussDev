if(TARGET pluss AND TARGET plussqit_codem)
  add_executable(plussma_optima_penalty_demo
    optima_penalty_demo.cpp
  )
  target_compile_features(plussma_optima_penalty_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_penalty_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_sequential_qp_demo
    optima_sequential_qp_demo.cpp
  )
  target_compile_features(plussma_optima_sequential_qp_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_sequential_qp_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_working_set_demo
    optima_working_set_demo.cpp
  )
  target_compile_features(plussma_optima_working_set_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_working_set_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_working_set_bench
    optima_working_set_bench.cpp
  )
  target_compile_features(plussma_optima_working_set_bench PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_working_set_bench PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_internal_protocol_demo
    optima_internal_protocol_demo.cpp
  )
  target_compile_features(plussma_optima_internal_protocol_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_internal_protocol_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_demo
    optima_demo.cpp
  )
  target_compile_features(plussma_optima_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_garch11_optima_demo
    garch11_optima_demo.cpp
  )
  target_compile_features(plussma_garch11_optima_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_garch11_optima_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_simple_garch11_optim_demo
    simple_garch11_optim_demo.cpp
  )
  target_compile_features(plussma_simple_garch11_optim_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_simple_garch11_optim_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_simple_garch11_objective_first_demo
    simple_garch11_objective_first_demo.cpp
  )
  target_compile_features(plussma_simple_garch11_objective_first_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_simple_garch11_objective_first_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_series_objective_analytic_grad_demo
    series_objective_analytic_grad_demo.cpp
  )
  target_compile_features(plussma_series_objective_analytic_grad_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_series_objective_analytic_grad_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_trace_objective_autodiff_demo
    trace_objective_autodiff_demo.cpp
  )
  target_compile_features(plussma_trace_objective_autodiff_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_trace_objective_autodiff_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_garch11_internal_protocol_demo
    garch11_internal_protocol_demo.cpp
  )
  target_compile_features(plussma_garch11_internal_protocol_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_garch11_internal_protocol_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_clone_parity_bench
    optima_clone_parity_bench.cpp
  )
  target_compile_features(plussma_optima_clone_parity_bench PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_clone_parity_bench PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_clone_parity_scorecard
    optima_clone_parity_scorecard.cpp
  )
  target_compile_features(plussma_optima_clone_parity_scorecard PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_clone_parity_scorecard PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_structured_clone_parity_bench
    optima_structured_clone_parity_bench.cpp
  )
  target_compile_features(plussma_optima_structured_clone_parity_bench PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_structured_clone_parity_bench PRIVATE pluss plussqit_codem)

  add_executable(plussma_optima_pathology_clone_parity_bench
    optima_pathology_clone_parity_bench.cpp
  )
  target_compile_features(plussma_optima_pathology_clone_parity_bench PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_pathology_clone_parity_bench PRIVATE pluss plussqit_codem)

  add_executable(plussma_garch11_nelder_mead_demo
    garch11_nelder_mead_demo.cpp
  )
  target_compile_features(plussma_garch11_nelder_mead_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_garch11_nelder_mead_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_pattern_search_demo
    pattern_search_demo.cpp
  )
  target_compile_features(plussma_pattern_search_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_pattern_search_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_dfo_suite_demo
    dfo_suite_demo.cpp
  )
  target_compile_features(plussma_dfo_suite_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_dfo_suite_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_garch11_dfo_demo
    garch11_dfo_demo.cpp
  )
  target_compile_features(plussma_garch11_dfo_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_garch11_dfo_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_mads_blackbox_demo
    mads_blackbox_demo.cpp
  )
  target_compile_features(plussma_mads_blackbox_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_mads_blackbox_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_mads_blackbox_native_demo
    mads_blackbox_native_demo.cpp
  )
  target_compile_features(plussma_mads_blackbox_native_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_mads_blackbox_native_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_dfo_unified_demo
    dfo_unified_demo.cpp
  )
  target_compile_features(plussma_dfo_unified_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_dfo_unified_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_adam_functional_demo
    adam_functional_demo.cpp
  )
  target_compile_features(plussma_adam_functional_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_adam_functional_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_septima_functional_demo
    septima_functional_demo.cpp
  )
  target_compile_features(plussma_septima_functional_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_septima_functional_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_septima_functional_multiblock_demo
    septima_functional_multiblock_demo.cpp
  )
  target_compile_features(plussma_septima_functional_multiblock_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_septima_functional_multiblock_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_simple_septima_functional_demo
    simple_septima_functional_demo.cpp
  )
  target_compile_features(plussma_simple_septima_functional_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_simple_septima_functional_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_simple_septima_functional_multiblock_demo
    simple_septima_functional_multiblock_demo.cpp
  )
  target_compile_features(plussma_simple_septima_functional_multiblock_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_simple_septima_functional_multiblock_demo PRIVATE pluss plussqit_codem)
endif()
