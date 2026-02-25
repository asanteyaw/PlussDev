if(TARGET pluss AND TARGET plussqit_codem)
  add_executable(plussma_garch11_nelder_mead_demo
    garch11_nelder_mead_demo.cpp
  )
  target_compile_features(plussma_garch11_nelder_mead_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_garch11_nelder_mead_demo PRIVATE pluss plussqit_codem)
endif()
