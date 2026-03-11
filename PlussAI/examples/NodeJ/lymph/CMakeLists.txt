set(NODEJ_LYMPH_DEMOS
  lymph_quick_classification_demo
  lymph_quick_regression_demo
  lymph_presets_demo
)

foreach(demo ${NODEJ_LYMPH_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
endforeach()
