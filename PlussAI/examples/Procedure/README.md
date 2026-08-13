# Procedure Demo Suite

These demos show Procedure as a PlussAI-level deployment graph, not a replacement
for Lymph, NodeJ, Ice, NodeK, or NodeB.

- `procedure_raw_series_parallel_demo.cpp`: raw Series branching, join, audit,
  and worker-capped local parallelism.
- `procedure_serial_series_demo.cpp`: a compact serial Series map chain using
  vectorized Series operations.
- `procedure_soft_failure_demo.cpp`: `fail_fast=false` behavior when an input
  artifact is missing.
- `procedure_row_guard_demo.cpp`: declared row-count-changing transforms and
  audit reporting.
- `procedure_future_guard_demo.cpp`: temporal-leakage guardrails for learning
  steps.
- `procedure_custom_module_demo.cpp`: a user-defined class with `forward(...)`
  consumed as a Procedure step.
- `procedure_dataframe_callable_demo.cpp`: a DataFrame callable step preserving
  frame shape.
- `procedure_labeled_model_demo.cpp`: direct Lymph `Model` train/infer through
  Procedure.
- `procedure_sink_chain_demo.cpp`: terminal sink behavior for deployable graph
  outputs.
- `procedure_result_artifact_demo.cpp`: typed result/artifact retrieval.
- `procedure_disable_audit_demo.cpp`: runtime audit suppression while keeping
  execution trace.
- `procedure_explicit_ports_demo.cpp`: explicit output/input port links between
  steps.
- `procedure_vector_text_artifact_demo.cpp`: non-NodeB text-vector artifacts in
  custom graph steps.
- `procedure_scalar_artifact_demo.cpp`: scalar artifact input/output handling.
- `procedure_empty_sink_demo.cpp`: sink behavior without an upstream artifact.
- `procedure_options_metadata_demo.cpp`: worker, schedule, and cache options on
  a Procedure.
- `procedure_multi_artifact_join_demo.cpp`: three-way join over Series
  artifacts.
- `procedure_safety_contract_demo.cpp`: required-column and time contracts with
  non-throwing failure reporting.
- `procedure_registry_persistence_demo.cpp`: registry-backed callable
  persistence and reload.
- `procedure_lymph_policy_payload_demo.cpp`: Lymph Policy and Payload admission
  with time roles preserved.
- `procedure_lymph_model_workflow_demo.cpp`: Lymph Workflow learn/infer through
  Procedure.
- `procedure_nodek_ice_demo.cpp`: raw NodeK module forwarding and Ice model
  learning through Procedure.
- `procedure_nodeb_text_demo.cpp`: NodeB NLP Workflow inside a Procedure graph.
- `procedure_bundle_audit_demo.cpp`: Procedure save/load with audit manifest
  inspection.
