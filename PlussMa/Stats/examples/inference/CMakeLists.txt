if(PLUSSMA_STATS_ENABLE_INFERENCE)
  add_executable(plussma_hypothesis_tests_demo
    hypothesis_tests_demo.cpp
  )
  target_link_libraries(plussma_hypothesis_tests_demo PRIVATE pluss)
  target_compile_features(plussma_hypothesis_tests_demo PRIVATE cxx_std_20)

  add_executable(plussma_matlab_parity_probe
    matlab_parity_probe.cpp
  )
  target_link_libraries(plussma_matlab_parity_probe PRIVATE pluss)
  target_compile_features(plussma_matlab_parity_probe PRIVATE cxx_std_20)
endif()
