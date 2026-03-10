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
