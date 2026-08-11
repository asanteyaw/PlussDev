set(LYMPH_DEMOS
  lymph_supervised_workflow_demo
  lymph_selection_search_demo
  lymph_general_ml_demo
)

foreach(demo ${LYMPH_DEMOS})
  add_executable(plussai_${demo} EXCLUDE_FROM_ALL
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_link_libraries(plussai_${demo} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
  target_include_directories(plussai_${demo} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/PlussMic/include
    ${PROJECT_SOURCE_DIR}/PlussMic/src
  )
endforeach()

set(LYMPH_SUITE_DEMOS
  suite/toy_workflow_regression.cpp
  suite/toy_workflow_classification.cpp
  suite/toy_builder_spotcheck.cpp
  suite/toy_search_workflow.cpp
  suite/toy_time_series_workflow.cpp
  suite/toy_shape_cluster_embed_detect.cpp
  suite/toy_semi_supervised.cpp
  suite/toy_custom_stage_payload_save.cpp
  suite/toy_policy_payload_gate.cpp
  suite/toy_reports_threshold_curve.cpp
  suite/toy_workflow_options_branch_factory.cpp
  suite/toy_kernel_one_class_detect.cpp
  suite/toy_neighbors_and_bayes.cpp
  suite/toy_series_batch_model.cpp
  suite/real_iris_multiclass_workflow.cpp
  suite/real_diabetes_classification_workflow.cpp
  suite/real_boston_housing_regression.cpp
  suite/real_boston_shape_gaussianize.cpp
  suite/real_abalone_mixed_regression.cpp
  suite/real_bank_marketing_spotcheck.cpp
  suite/real_online_news_search.cpp
  suite/real_steel_load_classification.cpp
  suite/real_stock_mtd_time_regression.cpp
  suite/real_sp500_time_regression.cpp
  suite/real_amsterdam_price_regression.cpp
  suite/real_breast_cancer_categorical.cpp
  suite/real_sonar_embedding_cluster.cpp
  suite/real_oil_spill_detection.cpp
  suite/real_oil_spill_classification.cpp
  suite/real_warehouse_sales_regression.cpp
  suite/real_ev_range_regression.cpp
  suite/real_accidents_severity_workflow.cpp
  suite/real_employee_salary_regression.cpp
  suite/real_estate_sale_regression.cpp
  suite/real_horse_colic_classification.cpp
  suite/real_stock_project_scout.cpp
)

foreach(demo_src ${LYMPH_SUITE_DEMOS})
  get_filename_component(demo_name ${demo_src} NAME_WE)
  add_executable(plussai_${demo_name} EXCLUDE_FROM_ALL
    ${demo_src}
  )
  target_link_libraries(plussai_${demo_name} PRIVATE pluss)
  target_link_libraries(plussai_${demo_name} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_compile_features(plussai_${demo_name} PRIVATE cxx_std_20)
  target_include_directories(plussai_${demo_name} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/PlussMic/include
    ${PROJECT_SOURCE_DIR}/PlussMic/src
  )
endforeach()
