set(NODEJ_LYMPH_DEMOS
  lymph_online_demo
  lymph_end_to_end_hour_workflow_showcase
  lymph_feature_transformer_workflow_demo
  lymph_housing_ch2_end_to_end_notebook
  lymph_api_reference_docverify_demo
  lymph_model_prep_io_docverify_demo
  lymph_validate_docverify_demo
  lymph_metrics_docverify_demo
  lymph_model_builder_docverify_demo
  lymph_models_builder_docverify_demo
  lymph_models_builder_benchmark_docverify_demo
  lymph_models_model_append_docverify_demo
  lymph_models_model_docverify_demo
  lymph_policy_docverify_demo
  lymph_tune_docverify_demo
  lymph_workflow_payload_docverify_demo
  lymph_prep_docverify_demo
  lymph_prep_column_router_docverify_demo
  lymph_prep_standard_scaler_docverify_demo
  lymph_prep_one_hot_encoder_docverify_demo
  lymph_prep_feature_transformer_docverify_demo
  lymph_prep_stage_docverify_demo
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
