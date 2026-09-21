# `pluss::ai::classic::FlowGraphInfo`

`FlowGraphInfo` is a read-only snapshot of a [`Flow`](Flow.md)'s structure and
runtime state — every [`TransformSpec`](TransformSpec.md) it holds, summarized
alongside whether the flow is trained and what kind of model (if any) it
carries. [`Flow::graph_info()`](Flow/graph_info.md) is the only way to obtain
one.

## Signature

```cpp
classic::StatusOr<classic::FlowGraphInfo> classic::Flow::graph_info() const;
```

## Construction

Obtained only from [`Flow::graph_info()`](Flow/graph_info.md). A plain value
type — copy it, store it, inspect it after the call returns; it does not
stay linked to the `Flow` it came from.

## State

`task` — `TaskKind`
: The attached model's task, or `Transform` for a model-less flow. Mirrors
  [`Flow::task_kind()`](Flow.md#state).

`trained` — `bool`
: Mirrors [`Flow::trained()`](Flow.md#state) at the moment `graph_info()` was
  called.

`has_model` — `bool`
: Mirrors [`Flow::has_model()`](Flow.md#state).

`has_opaque_transforms` — `bool`
: Mirrors [`Flow::has_opaque_transforms()`](Flow.md#state).

`stages` — `std::vector<FlowStageInfo>`
: One entry per transform stage, in order. Each `FlowStageInfo` is a small
  value type with its own five fields:

  `index` — `std::int64_t`
  : The stage's position, `0`-based.

  `stage_key` — `std::string`
  : The stage's name, or empty if none was set — see
    [`TransformSpec::stage_key`](TransformSpec.md#state).

  `kind` — `TransformKind`
  : Which [`prep`](../prep.md) stage this is.

  `routed_feature_count` — `std::int64_t`
  : How many columns this stage's route names. `0` when the stage has no
    explicit route (it sees every column).

  `remainder` — `RouteRemainderPolicy`
  : `PassThrough` or `Drop` — see [`TransformSpec::remainder`](TransformSpec.md#state).

  `route_active` — `bool`
  : `true` when `feature_route` is non-empty (the stage is routed to a
    specific subset of columns); `false` when it sees every column.

## Methods

None — `FlowGraphInfo` is a snapshot value, read directly. Call
[`Flow::graph_info()`](Flow/graph_info.md) again for a fresh one after the
flow's structure changes.

## Practical Example

A two-stage flow with a model, inspected through `graph_info()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});

  const cls::FlowGraphInfo info = flow.graph_info().value();
  std::cout << "task = " << static_cast<int>(info.task) << "\n";
  std::cout << "trained = " << std::boolalpha << info.trained << "\n";
  std::cout << "has_model = " << info.has_model << "\n";
  std::cout << "stage count = " << info.stages.size() << "\n";
  for (const auto& stage : info.stages) {
    std::cout << "  stage " << stage.index << ": kind = " << static_cast<int>(stage.kind)
              << ", routed_feature_count = " << stage.routed_feature_count
              << ", route_active = " << stage.route_active << "\n";
  }

  return 0;
}
```

Output

```text
task = 0
trained = false
has_model = true
stage count = 2
  stage 0: kind = 0, routed_feature_count = 1, route_active = true
  stage 1: kind = 6, routed_feature_count = 1, route_active = true
```

## Changelog

- **0.1** — `task`, `trained`, `has_model`, `has_opaque_transforms`, `stages`
  (a `FlowStageInfo` list).

## See Also

- [`Flow::graph_info`](Flow/graph_info.md) — the method that produces this.
- [`TransformSpec`](TransformSpec.md) — the declarative record each
  `FlowStageInfo` summarizes.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
