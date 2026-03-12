set(NODEJ_LYMPH_DEMOS
  lymph_quick_classification_demo
  lymph_quick_regression_demo
  lymph_presets_demo
  lymph_cv_tune_demo
  lymph_flow_demo
  lymph_multiclass_demo
  lymph_online_demo
)

foreach(demo ${NODEJ_LYMPH_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_link_libraries(plussai_${demo} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
endforeach()
