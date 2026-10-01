<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9d3657e6d98 -->
# `pluss::ai::classic::Flow::set_model`

`set_model` attaches (or replaces) a model built from a
[`ModelSpec`](../../families/ModelSpec.md) — the same construction
[`make_flow`](../make_flow.md) uses internally, callable directly on an
existing `Flow` so you can build the transform pipeline first and attach the
model afterward.

## Signature

```cpp
classic::Status classic::Flow::set_model(const classic::ModelSpec& spec);
```

## What It Does

Dispatches to [`make_classifier`](../../estimators/make_classifier.md),
[`make_regressor`](../../estimators/make_regressor.md), or
[`make_transformer`](../../estimators/make_transformer.md) based on
`spec.task`, and attaches (or replaces) the resulting model on this
`Flow` -- the same construction [`make_flow`](../make_flow.md) performs at
creation time, callable here so the transform pipeline can be built first
and the model attached afterward. A `Transform`-task model runs as the
last transform stage, so it counts toward `stage_count()` too.

## Parameters

`spec` — `ModelSpec`
: `{task, family, options}`. Dispatches to
  [`make_classifier`](../../estimators/make_classifier.md),
  [`make_regressor`](../../estimators/make_regressor.md), or
  [`make_transformer`](../../estimators/make_transformer.md) depending on
  `spec.task` — every error those three can return propagates here
  unchanged.

## Returns

A `Status`. `.ok()` on success — [`has_model()`](../Flow.md#state),
[`has_model_spec()`](../Flow.md#state), and [`model_spec()`](../Flow.md#state)
all reflect the new model immediately, for every `task`. A `Transform`-task
model additionally counts toward [`stage_count()`](../Flow.md#state), since
it runs through [`map`](map.md)/[`learn_map`](learn_map.md) exactly like a
transform stage — see Example 2.

`set_model` always replaces cleanly: calling it again — with any `task`, or
via [`set_classifier`](set_classifier.md)/[`set_regressor`](set_regressor.md)
— removes whatever model was there before, transform-task ones included. See
[`clear_model`](clear_model.md).

## Errors

- Whatever [`make_classifier`](../../estimators/make_classifier.md#errors)/
  [`make_regressor`](../../estimators/make_regressor.md#errors)/
  [`make_transformer`](../../estimators/make_transformer.md#errors) can
  return for the matching `task`.
- `Status::precondition`, from a *later* stage-adding call — not from
  `set_model` itself: once `set_model` has attached a `Transform`-task model,
  [`add_stage`](add_stage.md)/[`add_named_transform`](add_named_transform.md)/
  [`add_column_stage`](add_column_stage.md)/[`append_transform`](append_transform.md)/
  [`add_function_transformer`](add_function_transformer.md)/
  [`add_feature_transformer`](add_feature_transformer.md) all refuse to add
  another stage after it. Add every transform stage *before* calling
  `set_model` with a `Transform`-task spec, or give that model its own
  `Flow`. See Example 3.

## Practical Examples

### Example 1

The common case: build the pipeline, then attach a Classification model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::Status s =
      flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
ok = true
has_model() = true
has_model_spec() = true
stage_count() = 1
```

### Example 2

A `Transform`-task spec: `has_model()` becomes `true` just like the other two
tasks, and `stage_count()` counts the model too — it runs as the last step of
the transform pipeline.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;

  cls::Flow flow{};
  const cls::Status s = flow.set_model(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "task_kind() == Transform: " << (flow.task_kind() == cls::TaskKind::Transform) << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
ok = true
has_model() = true
has_model_spec() = true
task_kind() == Transform: true
stage_count() = 1
```

### Example 3

Ordering matters for a `Transform`-task spec: attach the model *after*
building every transform stage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;
  const cls::ModelSpec spec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster};

  // Correct order: stage(s) first, model last.
  cls::Flow ok_order{};
  ok_order.add_standard_scaler({}, {0});
  const cls::Status s1 = ok_order.set_model(spec);
  std::cout << "stage-then-model ok = " << std::boolalpha << s1.ok() << "\n";
  std::cout << "stage-then-model stage_count() = " << ok_order.stage_count() << "\n";

  // Reversed order: model first, then a stage -- rejected.
  cls::Flow bad_order{};
  bad_order.set_model(spec);
  const cls::Status s2 = bad_order.add_standard_scaler({}, {0});
  std::cout << "model-then-stage ok = " << s2.ok() << "\n";
  std::cout << "model-then-stage message = \"" << s2.message << "\"\n";

  return 0;
}
```

Output

```text
stage-then-model ok = true
stage-then-model stage_count() = 2
model-then-stage ok = false
model-then-stage message = "Flow::add_named_transform cannot add a stage after a TaskKind::Transform model has been set via set_model(); build every transform stage before attaching a Transform-task model, or attach the model to a separate Flow."
```

## See Also

- [`make_flow`](../make_flow.md) — builds a `Flow` and calls this in one
  step.
- [`set_classifier`](set_classifier.md) / [`set_regressor`](set_regressor.md)
  — attach an already-built model object instead of a `ModelSpec`.
- [`clear_model`](clear_model.md) — removes whatever model `set_model`
  attached, for every `task`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_classifier.html">
    <span class="label">Previous</span>
    <span class="title">set_classifier</span>
  </a>
  <a class="next-link" href="set_nested_param.html">
    <span class="label">Next</span>
    <span class="title">set_nested_param</span>
  </a>
</div>
<!-- apiref:nav:end -->
