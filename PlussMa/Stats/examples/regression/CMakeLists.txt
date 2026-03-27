if(PLUSSMA_STATS_ENABLE_REGRESSION)
  add_executable(plussma_ols_summary_demo
    ols_summary_demo.cpp
  )
  target_link_libraries(plussma_ols_summary_demo PRIVATE pluss)
  target_compile_features(plussma_ols_summary_demo PRIVATE cxx_std_20)

  add_executable(plussma_glm_summary_demo
    glm_summary_demo.cpp
  )
  target_link_libraries(plussma_glm_summary_demo PRIVATE pluss)
  target_compile_features(plussma_glm_summary_demo PRIVATE cxx_std_20)

  add_executable(plussma_quantile_summary_demo
    quantile_summary_demo.cpp
  )
  target_link_libraries(plussma_quantile_summary_demo PRIVATE pluss)
  target_compile_features(plussma_quantile_summary_demo PRIVATE cxx_std_20)

  add_executable(plussma_poly_summary_demo
    poly_summary_demo.cpp
  )
  target_link_libraries(plussma_poly_summary_demo PRIVATE pluss)
  target_compile_features(plussma_poly_summary_demo PRIVATE cxx_std_20)

  add_executable(plussma_bike_sharing_regression_notebook_demo
    bike_sharing_regression_notebook_demo.cpp
  )
  target_link_libraries(plussma_bike_sharing_regression_notebook_demo PRIVATE pluss PlussMic)
  target_compile_features(plussma_bike_sharing_regression_notebook_demo PRIVATE cxx_std_20)
  target_include_directories(plussma_bike_sharing_regression_notebook_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/PlussMic/include
    ${CMAKE_SOURCE_DIR}/PlussMic/src
    ${CMAKE_SOURCE_DIR}/PlussPl/include
  )
  target_compile_definitions(plussma_bike_sharing_regression_notebook_demo PRIVATE
    PLUSSDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  )
endif()
