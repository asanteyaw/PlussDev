# `pluss::ai::classic::TransformSpec`

`TransformSpec` is the declarative record of one [`Flow`](Flow.md) stage:
which transform kind, its options, which feature columns it is routed to,
and what happens to the columns it is not routed to. Every `add_*` method on
`Flow` builds one of these internally;
[`Flow::transform_specs()`](Flow.md#state) reads them back, and
[`graph_info()`](Flow/graph_info.md) summarizes them alongside the flow's
runtime state.

## Signature

```cpp
using classic::TransformOptions = /* a variant holding any one prep::*Options value */;
```

`TransformKind` is an enum with one value per [`prep`](../prep.md) stage this
reference documents — `StandardScaler`, `MinMaxScaler`, `RobustScaler`,
`SimpleImputer`, `NeighborImputer`, `IterativeImputer`, `OneHotEncoder`,
`FrequencyEncoder`, `VectorNormalizer`, `MaxAbsScaler`, `QuantileMap`,
`PowerMap`, `ThresholdBinarizer`, `PolynomialLift`, `OrdinalEncoder`,
`LabelEncoder`, `BaseNEncoder`, `HashingEncoder`, `TargetEncoder`,
`WOEEncoder`. `RouteRemainderPolicy` is a two-value enum, `PassThrough` /
`Drop` — see the `remainder` field below.

## Construction

You do not build a `TransformSpec` directly in ordinary code — it is what
[`add_named_transform`](Flow/add_named_transform.md),
[`add_column_stage`](Flow/add_column_stage.md), and every typed convenience
method on [`add_stage`](Flow/add_stage.md) construct and append to the flow
for you. Read it back through
[`Flow::transform_specs()`](Flow.md#state) or
[`graph_info()`](Flow/graph_info.md).

## State

`kind` — `TransformKind`
: Which [`prep`](../prep.md) stage this is — the enum value corresponding to
  the typed method that built it (`add_standard_scaler` sets
  `TransformKind::StandardScaler`, and so on).

`options` — `TransformOptions`
: The stage's own options struct, held in a `std::variant` of every
  `prep::*Options` type. The active alternative always matches `kind`.

`feature_route` — `std::vector<std::int64_t>`
: The 0-based column indices this stage sees. Empty means "every column" —
  the default for every `add_*` call that does not pass an explicit route.

`remainder` — `RouteRemainderPolicy`
: What happens to columns `feature_route` does not name: `PassThrough` (the
  default — they continue unchanged into whatever comes next) or `Drop`
  (they are removed from the flow's output entirely).

`stage_key` — `std::string`
: An optional name for the stage, settable at construction (via
  [`add_column_stage`](Flow/add_column_stage.md)) or afterward (via
  [`set_stage_key`](Flow/set_stage_key.md)). Empty by default. Used to refer
  to a specific stage in [`nested_param`](Flow/nested_param.md) paths.

## Methods

None — `TransformSpec` is five public fields, read directly. `Flow`'s own
methods are how you build and mutate the list of them a flow holds; see
[`Flow`](Flow.md#methods).

## Practical Example

Adding two routed stages and reading their `TransformSpec`s back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});               // column 0 only
  flow.add_one_hot_encoder({}, {1});                // column 1 only

  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  std::cout << "stage count = " << specs.size() << "\n";
  for (const cls::TransformSpec& spec : specs) {
    std::cout << "kind = " << static_cast<int>(spec.kind)
              << ", route size = " << spec.feature_route.size()
              << ", remainder = " << static_cast<int>(spec.remainder) << "\n";
  }

  return 0;
}
```

Output

```text
stage count = 2
kind = 0, route size = 1, remainder = 0
kind = 6, route size = 1, remainder = 0
```

## Changelog

- **0.1** — `kind`, `options`, `feature_route`, `remainder`, `stage_key`.

## See Also

- [`Flow`](Flow.md) — the pipeline this describes one stage of.
- [`add_stage`](Flow/add_stage.md) / [`add_named_transform`](Flow/add_named_transform.md) /
  [`add_column_stage`](Flow/add_column_stage.md) — what builds one.
- [`FlowGraphInfo`](FlowGraphInfo.md) — the runtime-state counterpart,
  combining this with whether each stage is trained.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
