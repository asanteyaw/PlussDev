<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e3aa45a3c6c1 -->

# `lymph::workflow::Workflow::map`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
Series Workflow::map(const FeatureBatch& batch, const InferOptions& options = {}) const;
Series Workflow::map(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const;
Series Workflow::map(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const InferOptions& options = {}) const;
Series Workflow::map(const Payload& payload, const InferOptions& options = {}) const;

Series Workflow::learn_map(
    const FeatureBatch& batch,
    const TrainOptions& learn_options = {}, const InferOptions& infer_options = {});
Series Workflow::learn_map(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const TrainOptions& learn_options = {}, const InferOptions& infer_options = {});
Series Workflow::learn_map(
    const pluss::table::DataFrame& raw_frame,
    const TrainOptions& learn_options = {}, const InferOptions& infer_options = {});
Series Workflow::learn_map(
    const Payload& payload,
    const TrainOptions& learn_options = {}, const InferOptions& infer_options = {});
```

## What It Does

`map` and `learn_map` run *preparation only* — attached prep stages (scalers,
encoders, imputers, custom transforms), whether attached through
[`on`](on.md) or `Model::append`/`prepare()` — with no estimator involved.
Both return the transformed feature matrix as a `Series`, not a prediction.

`map` (`const`) replays preparation that is **already fitted** — from a prior
`learn`/`learn_`/`learn_map` call. It never fits anything itself.

`learn_map` (not `const` — it always mutates the receiver, with no separate
`_`/value-returning pair) **fits** the attached prep stages, then maps in one
call — the estimator is left untouched, so `trained()` stays `false`
afterward even though `stage_count()` becomes non-zero (Example 2). Reach for
it for a purely transform-style pipeline (a scaler/encoder chain, or an
unsupervised model such as PCA) where there is no label to fit an estimator
against.

## Input Shapes

Same four shapes as [`learn`](learn.md) minus the `(x, Series y)` /
`(x, Thread y)` pair (mapping needs no label): `batch` (a pre-assembled
`FeatureBatch`), `raw_frame` alone or with `data_options` (naming feature
columns explicitly), and `payload` (a [`Payload`](../Payload.md) — its
`feature_batch()`/frame is used; a label role on it, if any, is ignored by
`map`/`learn_map`). Every raw-frame shape runs `Policy::apply(...)` first,
exactly as `learn` does.

`options` (`lymph::options::Infer`, on `map`) and `infer_options`
(`lymph::options::Infer`, on `learn_map`) are the same options type, unchanged
from [`infer`](infer.md) — see its Parameters section for the full field
list. `positive_threshold` and `unknown_categorical` are the fields most
relevant to a prep-only call.

`learn_options` (`lymph::options::Train`, on `learn_map`) is unchanged from
[`learn`](learn.md) — see its Parameters section.

## Returns

The transformed feature matrix, one row per input row (or per surviving row,
if a `MissingPolicy::Drop` dropped some). `learn_map` additionally leaves the
receiving `Workflow`'s prep stages fitted — `stage_count()` becomes non-zero
— without touching the held `Model`'s estimator.

## Errors

- `map` throws if any stage attached through `on(...)`/`append(...)` is still
  **pending** — never resolved and fitted by a prior `learn`/`learn_`/
  `learn_map` call. Rather than silently skip the stage and return raw
  features, `map` fails loudly, naming the pending-stage count (Example 3).
  Call `learn_`/`learn`/`learn_map` first.
- The bare `FeatureBatch` overload of `learn_map` throws under the same
  pending-stage condition — a `FeatureBatch` carries no column names, so a
  pending stage's route has nothing to resolve against there either. The
  `raw_frame`/`payload` overloads instead resolve pending stages
  automatically (Example 2) — the same resolution `learn_(frame, ...)` does.
- Every error `infer` itself can raise for its input shapes (a feature
  column missing from the frame, an unseen category under the default
  `unknown_categorical = Error`) applies to `map`/`learn_map` unchanged.

## Practical Examples

### Example 1

`map`, after a normal `learn`, replays the fitted scaler and returns the
transformed features — useful for inspecting exactly what the estimator
saw.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
      {"target", Thread(pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const workflow::Workflow trained = wf.learn(raw, data_options);

  const pluss::Series prepared = trained.map(raw, data_options);
  std::cout << "map -> " << prepared << "\n";

  return 0;
}
```

Output

```text
map -> pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-1.3416408, -1.3416408],
  [-0.4472136, -0.4472136],
  [0.4472136, 0.4472136],
  [1.3416408, 1.3416408]]
```

### Example 2

`learn_map` on a workflow with a still-**pending** routed stage: it resolves
and fits the stage, then maps, in one call. `trained()` stays `false` — only
prep was fit, not an estimator.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const pluss::Series mapped = wf.learn_map(raw);
  std::cout << "learn_map -> " << mapped << "\n";
  std::cout << "stage_count=" << wf.stage_count() << " trained=" << std::boolalpha << wf.trained() << "\n";

  return 0;
}
```

Output

```text
learn_map -> pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-1.3416408, -1.3416408],
  [-0.4472136, -0.4472136],
  [0.4472136, 0.4472136],
  [1.3416408, 1.3416408]]
stage_count=1 trained=false
```

### Example 3

`map` (unlike `learn_map`) never fits anything — a routed stage that has
never been resolved by a prior `learn`/`learn_`/`learn_map` call is still
pending, and `map` fails loudly rather than silently passing raw features
through.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  try {
    (void)wf.map(raw);
    std::cout << "map: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "map threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
map threw: map: 1 append()'d stage(s) still pending -- call learn_/learn (or, for a DataFrame input, learn_map) first so their ColumnRouter is resolved against real column names.
```

## See Also

- [`learn`](learn.md) — fits prep *and* the estimator.
- [`infer`](infer.md) — prep *and* the estimator's prediction.
- [`on`](on.md) — attaches the routed stages `map`/`learn_map` replay.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="on.html">
    <span class="label">Next</span>
    <span class="title">on</span>
  </a>
</div>
<!-- apiref:nav:end -->
