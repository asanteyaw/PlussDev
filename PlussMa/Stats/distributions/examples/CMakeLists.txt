if(TARGET pluss AND TARGET plussqit_codem)
  add_executable(plussma_distributions_catalog_demo
    distributions_catalog_demo.cpp
  )
  target_compile_features(plussma_distributions_catalog_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_distributions_catalog_demo PRIVATE pluss plussqit_codem)

  add_executable(plussma_distributions_objects_demo
    distributions_objects_demo.cpp
  )
  target_compile_features(plussma_distributions_objects_demo PRIVATE cxx_std_20)
  target_link_libraries(plussma_distributions_objects_demo PRIVATE pluss plussqit_codem)
endif()
