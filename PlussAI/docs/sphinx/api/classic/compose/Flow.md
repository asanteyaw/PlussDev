# `pluss::ai::classic::Flow`

`Flow` is a routed transform pipeline with an optional final model, fit and
driven as one unit. It is move-only (no copy constructor — use
[`clone`](Flow/clone.md) for an explicit deep copy) and aliased `Workflow`;
[`make_flow`](make_flow.md) (aliased `make_workflow`) is the usual way to
obtain one, seeded with a model from a [`ModelSpec`](../families/ModelSpec.md).

## Signature

```cpp
classic::Flow::Flow();                          // empty flow, no model
classic::Flow::Flow(classic::Flow&&) noexcept;   // move-constructible
classic::Flow& classic::Flow::operator=(classic::Flow&&) noexcept;
// Flow(const Flow&) and operator=(const Flow&) are deleted -- see clone().
```

## Construction

`Flow{}` builds an empty flow — no stages, no model. In practice you almost
always start from [`make_flow(spec)`](make_flow.md), which seeds the model
from a `ModelSpec` so you only need to add transform stages before `learn`.
An empty `Flow{}` is still useful on its own as a *transform-only* pipeline
(no model attached) — see [`clear_model`](Flow/clear_model.md) and
[`map`](Flow/map.md).

## State

Six cheap, `noexcept` accessors read the flow's current structure without a
`Status` wrapper — call any of them at any point in the lifecycle, including
on an empty, just-constructed `Flow`:

`trained()` — `bool`
: `true` once `learn` (or [`learn_map`](Flow/learn_map.md), or
  [`mark_trained(true)`](Flow/mark_trained.md)) has succeeded. `false` on a
  fresh `Flow` and after [`clear_model`](Flow/clear_model.md) /
  [`clear_transforms`](Flow/clear_transforms.md).

`has_model()` — `bool`
: `true` once a concrete model exists on the flow — immediately after
  [`set_classifier`](Flow/set_classifier.md) /
  [`set_regressor`](Flow/set_regressor.md), or after
  [`set_model`](Flow/set_model.md) / [`make_flow`](make_flow.md) for any
  `ModelSpec` task, `Transform` included (all build the estimator right
  away, before any `learn` call — `trained()` is what tracks whether it has
  been fit).

`has_model_spec()` — `bool`
: `true` once a `ModelSpec` has been set (via
  [`set_model`](Flow/set_model.md) or [`make_flow`](make_flow.md)) — distinct
  from `has_model()`, which is about the *built* estimator object, not the
  spec describing it.

`has_opaque_transforms()` — `bool`
: `true` once [`append_transform`](Flow/append_transform.md) has attached a
  hand-built `TransformerBase` rather than a stage the flow built from a
  `TransformKind`. An opaque-transform flow cannot be
  [`clone`](Flow/clone.md)d or round-tripped through
  [`save_blueprint`](Flow/save_blueprint.md) — see those pages.

`task_kind()` — `TaskKind`
: The attached model's task (`Classification`, `Regression`, or `Transform`
  for a flow with no model / a transform-only model). Mirrors
  [`EstimatorBase::task_kind`](../estimators/EstimatorBase/task_kind.md) for
  the model this flow wraps.

`stage_count()` — `std::int64_t`
: The number of transform stages added so far (via any `add_*` method or
  `append_transform`), plus one more if a `Transform`-task model is attached
  — it runs as the pipeline's last step, exactly like a stage. A
  Classification/Regression model does not count here.

Two further accessors return the underlying data directly rather than a
derived query: `model_spec()` (`const std::optional<ModelSpec>&`) and
`transform_specs()` (`const std::vector<TransformSpec>&`). Most code reaches
for [`graph_info()`](Flow/graph_info.md) instead — the same information,
already assembled into one read-only snapshot.

## Methods

**Pipeline construction**

| Method | Covered on |
| --- | --- |
| `add_standard_scaler`, `add_one_hot_encoder`, … (20 typed methods) | [`add_stage`](Flow/add_stage.md) |
| `add_named_transform(kind, options, ...)` | [`add_named_transform`](Flow/add_named_transform.md) |
| `add_column_stage(stage_key, kind, options, ...)` | [`add_column_stage`](Flow/add_column_stage.md) |
| `add_function_transformer(transformer)` | [`add_function_transformer`](Flow/add_function_transformer.md) |
| `add_feature_transformer(transformer)` | [`add_feature_transformer`](Flow/add_feature_transformer.md) |
| `append_transform(transformer)` | [`append_transform`](Flow/append_transform.md) |
| `clear_transforms()` | [`clear_transforms`](Flow/clear_transforms.md) |

**Stage mutation**

| Method | Covered on |
| --- | --- |
| `set_stage_key(index, key)` | [`set_stage_key`](Flow/set_stage_key.md) |
| `set_stage_route(index, route)` | [`set_stage_route`](Flow/set_stage_route.md) |
| `set_stage_remainder(index, policy)` | [`set_stage_remainder`](Flow/set_stage_remainder.md) |

**Model attachment**

| Method | Covered on |
| --- | --- |
| `set_model(spec)` | [`set_model`](Flow/set_model.md) |
| `set_classifier(model)` / `set_regressor(model)` | [`set_classifier`](Flow/set_classifier.md) / [`set_regressor`](Flow/set_regressor.md) |
| `clear_model()` | [`clear_model`](Flow/clear_model.md) |

**Fit, predict, score**

| Method | Covered on |
| --- | --- |
| `learn(batch or frame, ...)` | [`learn`](Flow/learn.md) |
| `map(batch, ...)` | [`map`](Flow/map.md) |
| `learn_map(batch, ...)` | [`learn_map`](Flow/learn_map.md) |
| `infer(batch or frame, ...)` | [`infer`](Flow/infer.md) |
| `infer_scores(batch or frame, ...)` | [`infer_scores`](Flow/infer_scores.md) |
| `assess(batch or frame, ...)` | [`assess`](Flow/assess.md) |

**Introspection**

| Method | Covered on |
| --- | --- |
| `trained()` / `has_model()` / `has_model_spec()` / `has_opaque_transforms()` / `task_kind()` / `stage_count()` | `## State`, above |
| `graph_info()` | [`graph_info`](Flow/graph_info.md) |
| `flow_summary()` | [`flow_summary`](Flow/flow_summary.md) |
| `nested_param_keys()` / `nested_param(path)` / `set_nested_param(path, value)` | [`nested_param`](Flow/nested_param.md) |
| `mark_trained(trained)` | [`mark_trained`](Flow/mark_trained.md) |

**Persistence**

| Method | Covered on |
| --- | --- |
| `dump_transform_states()` / `load_transform_states(states)` | [`dump_transform_states`](Flow/dump_transform_states.md) |
| `dump_model_state()` / `load_model_state(state)` | [`dump_model_state`](Flow/dump_model_state.md) |
| `save_blueprint()` / `load_blueprint(text)` | [`save_blueprint`](Flow/save_blueprint.md) |
| `clone()` | [`clone`](Flow/clone.md) |

## Changelog

- **0.1** — routed multi-stage transform pipeline, optional final model,
  blueprint + state persistence.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Flow/Introduction
Flow/add_column_stage
Flow/add_feature_transformer
Flow/add_function_transformer
Flow/add_named_transform
Flow/add_stage
Flow/append_transform
Flow/assess
Flow/clear_model
Flow/clear_transforms
Flow/clone
Flow/dump_model_state
Flow/dump_transform_states
Flow/flow_summary
Flow/graph_info
Flow/infer
Flow/infer_scores
Flow/learn
Flow/learn_map
Flow/load_blueprint
Flow/load_model_state
Flow/load_transform_states
Flow/map
Flow/mark_trained
Flow/nested_param
Flow/nested_param_keys
Flow/save_blueprint
Flow/set_classifier
Flow/set_model
Flow/set_nested_param
Flow/set_regressor
Flow/set_stage_key
Flow/set_stage_remainder
Flow/set_stage_route
```
<!-- apiref:toctree:end -->
