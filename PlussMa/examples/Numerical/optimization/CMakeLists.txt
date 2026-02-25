if(TARGET pluss AND TARGET plussqit_codem)
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
endif()
