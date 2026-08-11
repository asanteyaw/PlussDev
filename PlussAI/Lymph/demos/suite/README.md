# Lymph Capability Demo Suite

This directory is a broad API-reference seed for the Lymph redesign. The demos
avoid helper functions and local abstraction layers so every file shows the raw
family surface: `Policy`, `Payload`, `Workflow`, `Model`, `Builder`,
`Validate`, `SpotCheck`, `Search`, `Reports`, `Shape`, `Cluster`, `Embed`,
`Detect`, and `Semi`.

The demo targets are `EXCLUDE_FROM_ALL` so the normal build is unchanged. Build
one explicitly, for example:

```sh
cmake --build build_cpp_env --target plussai_real_iris_multiclass_workflow
```

## Synthetic Demos

- `toy_workflow_regression.cpp`: generated Series data, branch Workflow,
  regression training, assessment, inference, and payload-backed save.
- `toy_workflow_classification.cpp`: generated classification data, branch
  preprocessing, reports, confusion table, and threshold search.
- `toy_builder_spotcheck.cpp`: named model coordination, benchmark, best/worst,
  and spotcheck top/bottom.
- `toy_search_workflow.cpp`: Workflow-aware hyperparameter search.
- `toy_time_series_workflow.cpp`: explicit temporal payload, time-aware
  Workflow, and time validation.
- `toy_shape_cluster_embed_detect.cpp`: gaussianizing profile/plan, k-means,
  PCA embedding, and model-backed anomaly detection.
- `toy_semi_supervised.cpp`: pseudo-labeling with a classifier model.
- `toy_custom_stage_payload_save.cpp`: custom Series stage, typed Payload, and
  deployment-shaped persistence.
- `toy_policy_payload_gate.cpp`: explicit schema admission through Policy before
  Workflow execution.
- `toy_reports_threshold_curve.cpp`: classification reports, threshold curves,
  and best-threshold selection.
- `toy_workflow_options_branch_factory.cpp`: construction-time branch specs and
  WorkflowOptions.
- `toy_kernel_one_class_detect.cpp`: one-class kernel detector through the
  `detect` family.
- `toy_neighbors_and_bayes.cpp`: neighbor and Bayes model factories inside
  Builder/SpotCheck.
- `toy_series_batch_model.cpp`: direct `LabeledBatch`/Series training without
  DataFrame detours.

## Real-Data Demos

- `real_iris_multiclass_workflow.cpp`
- `real_diabetes_classification_workflow.cpp`
- `real_boston_housing_regression.cpp`
- `real_boston_shape_gaussianize.cpp`
- `real_abalone_mixed_regression.cpp`
- `real_bank_marketing_spotcheck.cpp`
- `real_online_news_search.cpp`
- `real_steel_load_classification.cpp`
- `real_stock_mtd_time_regression.cpp`
- `real_sp500_time_regression.cpp`
- `real_amsterdam_price_regression.cpp`
- `real_breast_cancer_categorical.cpp`
- `real_sonar_embedding_cluster.cpp`
- `real_oil_spill_detection.cpp`
- `real_oil_spill_classification.cpp`
- `real_warehouse_sales_regression.cpp`
- `real_ev_range_regression.cpp`
- `real_accidents_severity_workflow.cpp`
- `real_employee_salary_regression.cpp`
- `real_estate_sale_regression.cpp`
- `real_horse_colic_classification.cpp`
- `real_stock_project_scout.cpp`

## Reference Material Read

- scikit-learn Pipeline docs:
  `https://scikit-learn.org/stable/modules/generated/sklearn.pipeline.Pipeline.html`
- scikit-learn Pipeline source:
  `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/pipeline.py`
- scikit-learn composite estimator guide:
  `https://scikit-learn.org/stable/modules/compose.html#pipeline`
- scikit-learn ColumnTransformer guide:
  `https://scikit-learn.org/stable/modules/compose.html#column-transformer`
- scikit-learn ColumnTransformer source:
  `https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/compose/_column_transformer.py`

The suite keeps Pluss-native names and semantics. The sklearn material was used
for correctness and capability comparison only.
