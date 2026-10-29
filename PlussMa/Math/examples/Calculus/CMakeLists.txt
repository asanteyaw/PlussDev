function(plussma_add_calculus_example source_file)
  get_filename_component(stem "${source_file}" NAME_WE)
  set(target_name "plussma_calculus_${stem}")
  add_executable(${target_name} "${source_file}")
  target_compile_features(${target_name} PRIVATE cxx_std_20)
  target_link_libraries(${target_name} PRIVATE pluss plussqit_codem)
  set_target_properties(${target_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/PlussMa/Math/examples/Calculus"
  )
endfunction()

plussma_add_calculus_example(calculus_demo.cpp)
plussma_add_calculus_example(calculus_benchmark.cpp)
