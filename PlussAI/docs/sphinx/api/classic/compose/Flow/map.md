<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a86e1049d8d7 -->

# `pluss::ai::classic::Flow::map`

`map` runs a `FeatureBatch` through every already-fitted transform stage, in
order, and returns the resulting `Series` — the model (if any) is not
consulted at all. It is the transform-only counterpart to
[`infer`](infer.md): `infer` calls `map` internally and then hands the result
to the model, while `map` on its own is what a transform-only `Flow` (no
model attached) uses to produce its output directly.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::map(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
```

## Parameters

`batch` — `FeatureBatch`
: The features to map through every stage's fitted transform, in the order
  they were added, each stage seeing only the columns its route names (or
  every column, if it has none — see [`add_stage`](add_stage.md)).

`options` — `InferOptions`, default `{}`
: Forwarded to every stage's own `map` call — missing-value handling and
  unknown-categorical handling apply per stage, not just once at the end. See
  [`InferOptions`](../../core/InferOptions.md).

## Returns

A `StatusOr<Series>` — the fully-transformed feature matrix, after every
stage has run. A `Flow` with no stages at all returns `batch.x` unchanged
(still validated first).

## Errors

- Any stage's own `map` failing propagates that stage's `Status` directly —
  most commonly `Status::precondition` if that stage (or the whole `Flow`)
  was never fit; see [`TransformerBase::map`](../../estimators/TransformerBase.md)
  for the shared per-kernel contract.
- `Status::shape_mismatch` if a routed stage's feature count does not match
  its `feature_route`, or if a stage's output does not preserve the sample
  count.
- `Status` from [`validate_features`](../../data/FeatureBatch.md) for an
  undefined or rank-1 `batch.x`, checked before any stage runs.

## Practical Examples

`map` on a fitted, transform-only `Flow`, and the same call failing before
`learn`/`learn_map` has fit anything.

### Example 1

A two-stage `Flow` (scale, then a one-hot encode routed to a different
column) with no model — `map` alone is the whole pipeline's output. The
encoded column widens from 1 to 3 (2 seen categories plus
`OneHotEncoder`'s default unknown bucket), landing after the scaled column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // no model -- transform-only
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});

  // column 0: numeric; column 1: a 2-category code (0 or 1).
  const cls::Series x = pluss::make_series(
      {10.0, 0.0, 20.0, 1.0, 30.0, 0.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  flow.learn_map(batch);  // fit both stages first
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "mapped = " << mapped.value() << "\n";

  return 0;
}
```

Output

```text
map ok = true
mapped = pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[-1.2247449, 1, 0, 0],
  [0, 0, 1, 0],
  [1.2247449, 1, 0, 0]]
```

### Example 2

`map` before any fit — every stage's own precondition fires, the same way a
lone [`StandardScaler`](../../prep/scaling/StandardScaler.md) would.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_standard_scaler();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "StandardScaler::map requires learn() before map()."
```

### Example 3

`map` bypasses the model even on a `Flow` that has one — it only ever runs
the transform stages. Compare against [`infer`](infer.md), which runs `map`
and then the model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({20.0}, {1}, f64).reshape({1, 1});
  std::cout << "map (scaled feature, not a prediction) = " << flow.map(probe).value() << "\n";
  std::cout << "infer (the actual prediction)          = " << flow.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
map (scaled feature, not a prediction) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
infer (the actual prediction)          = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[2]
```

## See Also

- [`learn_map`](learn_map.md) — fit and map the same data in one call.
- [`infer`](infer.md) — `map` followed by the attached model's `infer`.
- [`add_stage`](add_stage.md) — adding the stages `map` runs through.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_transform_states.html">
    <span class="label">Previous</span>
    <span class="title">load_transform_states</span>
  </a>
  <a class="next-link" href="mark_trained.html">
    <span class="label">Next</span>
    <span class="title">mark_trained</span>
  </a>
</div>
<!-- apiref:nav:end -->
