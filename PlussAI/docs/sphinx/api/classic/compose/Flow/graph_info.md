<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9698cc82f254 -->

# `pluss::ai::classic::Flow::graph_info`

`graph_info` returns a [`FlowGraphInfo`](../FlowGraphInfo.md) — a read-only
snapshot of the flow's current structure and runtime state, assembled from
`task_kind()`, `trained()`, `has_model()`, `has_opaque_transforms()`, and one
`FlowStageInfo` per transform stage. It is the one call that hands back every
introspection field at once, rather than querying each accessor
individually.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<classic::FlowGraphInfo> classic::Flow::graph_info() const;
```

## What It Does

Gathers every introspection field this `Flow` exposes -- `task_kind()`,
`trained()`, `has_model()`, `has_opaque_transforms()`, and one
`FlowStageInfo` per transform stage -- into a single
[`FlowGraphInfo`](../FlowGraphInfo.md) value, in one call. Reach for it
when several of those facts are needed together, rather than querying each
accessor separately.

## Parameters

None.

## Returns

A `StatusOr<FlowGraphInfo>` — always successful in practice (there is no
input to validate), returned as a `StatusOr` for interface consistency with
every other `Flow` query that can fail. See
[`FlowGraphInfo`](../FlowGraphInfo.md) for what every field means, including
the worked example there.

## Errors

None — `graph_info()` cannot fail; it only reads state already held by the
`Flow`.

## Practical Examples

### Example 1

`graph_info()` called at three points in a flow's lifecycle — empty, staged,
and trained — showing how the snapshot tracks state as it changes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  auto snapshot = [&](const char* label) {
    const cls::FlowGraphInfo info = flow.graph_info().value();
    std::cout << label << ": stages=" << info.stages.size()
              << " has_model=" << std::boolalpha << info.has_model
              << " trained=" << info.trained << "\n";
  };

  snapshot("empty          ");
  flow.add_standard_scaler();
  snapshot("stage added    ");

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  flow.learn_map(batch);  // no model attached -- this alone reaches trained()
  snapshot("stage learned  ");

  return 0;
}
```

Output

```text
empty          : stages=0 has_model=false trained=false
stage added    : stages=1 has_model=false trained=false
stage learned  : stages=1 has_model=false trained=true
```

### Example 2

`has_opaque_transforms` flips to `true` only for an
[`append_transform`](append_transform.md) stage, distinguishing it from a
stage added by kind.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow named;
  named.add_standard_scaler();
  std::cout << "named stage:  has_opaque_transforms = " << std::boolalpha
            << named.graph_info().value().has_opaque_transforms << "\n";

  cls::Flow opaque;
  opaque.append_transform(std::make_unique<cls::prep::StandardScaler>());
  std::cout << "append_transform: has_opaque_transforms = "
            << opaque.graph_info().value().has_opaque_transforms << "\n";

  return 0;
}
```

Output

```text
named stage:  has_opaque_transforms = false
append_transform: has_opaque_transforms = true
```

### Example 3

Per-stage fields (`kind`, `routed_feature_count`, `route_active`) for two
stages with different routing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler({}, {0});   // routed to column 0
  flow.add_one_hot_encoder();          // no route -- sees every column

  const cls::FlowGraphInfo info = flow.graph_info().value();
  for (const auto& stage : info.stages) {
    std::cout << "stage " << stage.index << ": kind=" << static_cast<int>(stage.kind)
              << " routed_feature_count=" << stage.routed_feature_count
              << " route_active=" << std::boolalpha << stage.route_active << "\n";
  }

  return 0;
}
```

Output

```text
stage 0: kind=0 routed_feature_count=1 route_active=true
stage 1: kind=6 routed_feature_count=0 route_active=false
```

## See Also

- [`FlowGraphInfo`](../FlowGraphInfo.md) — the returned snapshot type, with
  every field documented.
- [`flow_summary`](flow_summary.md) — the same information as a
  human-readable string instead of a structured value.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow_summary.html">
    <span class="label">Previous</span>
    <span class="title">flow_summary</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
