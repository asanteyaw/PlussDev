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

  add_executable(plussma_optima_interior_point_demo
    optima_interior_point_demo.cpp
  )
  target_compile_features(plussma_optima_interior_point_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_interior_point_demo PRIVATE pluss plussqit_codem)

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

  add_executable(plussma_optima_clone_parity_bench
    optima_clone_parity_bench.cpp
  )
  target_compile_features(plussma_optima_clone_parity_bench PRIVATE cxx_std_20)
  target_link_libraries(plussma_optima_clone_parity_bench PRIVATE pluss plussqit_codem)

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
endif()
