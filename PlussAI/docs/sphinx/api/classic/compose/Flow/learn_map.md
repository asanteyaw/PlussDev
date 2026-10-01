<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d619c6cc7d03 -->

# `pluss::ai::classic::Flow::learn_map`

`learn_map` fits every transform stage and maps the same batch through them,
in one call — the stage-only counterpart to [`learn`](learn.md). The model
(if any) is never touched: `learn_map` is what a transform-only `Flow` uses
to both fit and produce the transformed representation of its training data
in one step, the same way [`TransformerBase::learn_map`](../../estimators/TransformerBase.md)
does for a single kernel.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<pluss::Series> classic::Flow::learn_map(
    const classic::FeatureBatch& batch,
    const classic::TrainOptions& learn_options = {},
    const classic::InferOptions& infer_options = {});
```

## What It Does

Fits every transform stage on `batch`, in order, then immediately maps
`batch` through those same newly-fitted stages and returns the result --
fit and transform, in one call, on the same data. Any attached model is
never touched; this is the transform-only counterpart to
[`learn`](learn.md), for a `Flow` used purely to produce a fitted
representation of its training data rather than to also train a final
model.

## Parameters

`batch` — `FeatureBatch`
: The features to fit every stage on and map through them.

`learn_options` — `TrainOptions`, default `{}`
: Forwarded to each stage's fit call.

`infer_options` — `InferOptions`, default `{}`
: Forwarded to each stage's map call, applying its fitted transform to the
  same batch it was just fit on.

## Returns

A `StatusOr<Series>` — the fitted stages' output on `batch`.

**`trained()` afterward depends on whether a model is attached, and this is
the one place `Flow`'s two roles genuinely diverge:**

- **No model attached** (`has_model() == false`): `trained()` becomes `true`
  — the stages are everything this `Flow` needs to fit, and they just did.
- **A model is attached** (`has_model() == true`): `trained()` stays
  `false` — only the *stages* fit here, and a model that has never seen the
  transformed data is not a trained `Flow` by this contract. Call
  [`learn`](learn.md) instead (which calls the model's own `learn` after
  fitting stages) to reach `trained() == true` on a `Flow` that carries a
  model.

## Errors

Identical to [`learn`](learn.md)'s stage-fitting errors — a stage's own
`learn_supervised`/`learn_map` failing propagates that `Status` directly, and
a supervised stage with no target attached fails with the same
"attach it inside `Flow::learn`" message, since `learn_map` has no target
parameter to give it one.

## Practical Examples

### Example 1

A transform-only `Flow`: `learn_map` alone reaches `trained() == true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // no model
  flow.add_standard_scaler();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});

  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "ok        = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() = " << flow.trained() << "\n";
  std::cout << "mapped    = " << mapped.value() << "\n";

  return 0;
}
```

Output

```text
ok        = true
trained() = true
mapped    = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.3416408],
  [-0.4472136],
  [0.4472136],
  [1.3416408]]
```

### Example 2

The divergence: the identical call on a `Flow` that carries a model —
stages fit, but `trained()` stays `false` until `learn` also fits the model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});

  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "has_model()          = " << std::boolalpha << flow.has_model() << "\n";
  std::cout << "learn_map ok         = " << mapped.ok() << "\n";
  std::cout << "trained() after      = " << flow.trained() << "  (model never fit)\n";

  return 0;
}
```

Output

```text
has_model()          = true
learn_map ok         = true
trained() after      = false  (model never fit)
```

### Example 3

Calling `learn_map` a second time re-fits from scratch — the second call's
statistics replace the first's rather than being blended with them.

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

  cls::FeatureBatch first{};
  first.x = pluss::make_series({0.0, 10.0}, {2}, f64).reshape({2, 1});
  const pluss::Series first_mapped = flow.learn_map(first).value();

  cls::FeatureBatch second{};
  second.x = pluss::make_series({100.0, 300.0}, {2}, f64).reshape({2, 1});
  const pluss::Series second_mapped = flow.learn_map(second).value();

  std::cout << "first learn_map  = " << first_mapped << "\n";
  std::cout << "second learn_map = " << second_mapped << "\n";

  return 0;
}
```

Output

```text
first learn_map  = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-1],
  [1]]
second learn_map = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-1],
  [1]]
```

## See Also

- [`learn`](learn.md) — fits stages *and* the model, the call that reaches
  `trained() == true` on a `Flow` with a model.
- [`map`](map.md) — apply already-fitted stages without refitting.
- [`TransformerBase::learn_map`](../../estimators/TransformerBase.md) — the
  single-kernel equivalent every stage implements.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="load_blueprint.html">
    <span class="label">Next</span>
    <span class="title">load_blueprint</span>
  </a>
</div>
<!-- apiref:nav:end -->
