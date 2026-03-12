set(NODEJ_LYMPH_DEMOS
  lymph_quick_classification_demo
  lymph_quick_regression_demo
  lymph_presets_demo
  lymph_cv_tune_demo
  lymph_flow_demo
  lymph_multiclass_demo
  lymph_online_demo
  lymph_unsupervised_demo
  lymph_end_to_end_hour_workflow_showcase
)

foreach(demo ${NODEJ_LYMPH_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_link_libraries(plussai_${demo} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
  target_compile_definitions(plussai_${demo} PRIVATE
    PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    PLUSSAI_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussAI"
  )
endforeach()
