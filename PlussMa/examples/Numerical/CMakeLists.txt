if(TARGET pluss AND TARGET plussqit_codem)
  add_executable(plussma_distributions_demo
    distributions_demo.cpp
  )
  target_compile_features(plussma_distributions_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_distributions_demo PRIVATE pluss plussqit_codem)
endif()

add_subdirectory(optimization)
