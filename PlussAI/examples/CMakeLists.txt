if(TARGET pluss)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeJ")
    add_subdirectory(NodeJ)
  endif()

  add_executable(plussai_nn_rnn_demo
    nn_rnn_demo.cpp
  )
  target_link_libraries(plussai_nn_rnn_demo PRIVATE pluss)
  target_compile_features(plussai_nn_rnn_demo PRIVATE cxx_std_20)

  add_executable(plussai_func_rnn_demo
    func_rnn_demo.cpp
  )
  target_link_libraries(plussai_func_rnn_demo PRIVATE pluss)
  target_compile_features(plussai_func_rnn_demo PRIVATE cxx_std_20)

  add_executable(plussai_func_conv_demo
    func_conv_demo.cpp
  )
  target_link_libraries(plussai_func_conv_demo PRIVATE pluss)
  target_compile_features(plussai_func_conv_demo PRIVATE cxx_std_20)

  add_executable(plussai_nn_conv_demo
    nn_conv_demo.cpp
  )
  target_link_libraries(plussai_nn_conv_demo PRIVATE pluss)
  target_compile_features(plussai_nn_conv_demo PRIVATE cxx_std_20)

  add_executable(plussai_tcn_demo
    tcn_demo.cpp
  )
  target_link_libraries(plussai_tcn_demo PRIVATE pluss)
  target_compile_features(plussai_tcn_demo PRIVATE cxx_std_20)

  add_executable(plussai_adam_oop_demo
    adam_oop_demo.cpp
  )
  target_link_libraries(plussai_adam_oop_demo PRIVATE pluss)
  target_compile_features(plussai_adam_oop_demo PRIVATE cxx_std_20)
endif()
