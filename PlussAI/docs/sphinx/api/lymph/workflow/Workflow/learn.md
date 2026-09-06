<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 77757ee003a9 -->

# `lymph::workflow::Workflow::learn`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
// In place: the left-hand Workflow's Model becomes the fitted artifact.
Workflow& Workflow::learn_(
    const LabeledBatch& batch, const TrainOptions& options = {});
Workflow& Workflow::learn_(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const TrainOptions& options = {});
Workflow& Workflow::learn_(
    const pluss::table::DataFrame& x, const Series& y,
    const TrainOptions& options = {});
Workflow& Workflow::learn_(
    const pluss::table::DataFrame& x, const pluss::table::Thread& y,
    const TrainOptions& options = {});
Workflow& Workflow::learn_(const Payload& payload, const TrainOptions& options = {});

// By value: `*this` is left untouched; the trained Workflow is returned.
Workflow Workflow::learn(
    const LabeledBatch& batch, const TrainOptions& options = {}) const;
Workflow Workflow::learn(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const TrainOptions& options = {}) const;
Workflow Workflow::learn(
    const pluss::table::DataFrame& x, const Series& y,
    const TrainOptions& options = {}) const;
Workflow Workflow::learn(
    const pluss::table::DataFrame& x, const pluss::table::Thread& y,
    const TrainOptions& options = {}) const;
Workflow Workflow::learn(const Payload& payload, const TrainOptions& options = {}) const;
```

## What `Workflow` Adds

`learn` on a `Workflow` is `Policy::apply(...)` (on any argument that carries
a raw frame) followed by the identically-shaped call on the held `Model`. The
actual fitting mechanics — attached prep stages fitted stage by stage in
attachment order, then the estimator fitted on the prepared features — are
entirely [`models::Model::learn`](../../models/Model/learn.md)'s concern;
this page only covers what changes one layer up.

The `LabeledBatch` overload skips `Policy` entirely — a `LabeledBatch` is
already past raw-frame admission, so there is nothing for `Policy` to apply
to. Every other overload (including `Payload` built from a frame) runs
`Policy::apply(...)` first.

## Input Shapes

Four of the five shapes are identical to `Model::learn`'s — see
[Input Shapes there](../../models/Model/learn.md#input-shapes) for the full
treatment of each. Summarized here, with every signature parameter named:

`batch`
: A pre-assembled `LabeledBatch`. Skips `Policy` (see above).

`raw_frame`, `data_options`
: A raw frame plus a `DataFrameBatchOptions` naming its feature and label
  columns — `Policy::apply(raw_frame)` runs first.

`x`, `y`
: Feature columns in a frame, label as a standalone `Series` or `Thread`.
  `Policy::apply(x)` runs first, subject to the `filter_nulls`-safety
  restriction in Errors below.

`payload`
: A [`Payload`](../Payload.md) — whichever shape it holds. A frame-backed
  `Payload` with a named label column (`.label(feature_name)`) routes through
  the `raw_frame`/`data_options` path; one with a `Series` label (`.label(y)`)
  routes through the `x`/`y` path (and is therefore subject to the same
  `filter_nulls`-safety restriction); a `FeatureBatch`/`LabeledBatch`-backed
  `Payload` skips `Policy` like the bare `batch` overload does.

## Parameters

`data_options` (`lymph::options::DFBatch`) and `options`
(`lymph::options::Train`) are unchanged from `Model::learn` — see
[its Parameters section](../../models/Model/learn.md#parameters) for the full
field list (`feature_names`, `label_feature`, `note_policy`,
`max_iterations`, `tolerance`, `warm_start`, `sample_weights`,
`class_weight`, `missing`, `random`, `balance`).

## Returns

Same in-place/by-value split as `Model::learn`: `learn_` returns a reference
to the receiver (now trained); `learn` returns a new `Workflow` and leaves the
receiver untouched. Either way the returned/mutated `Workflow` keeps this
`Workflow`'s `Policy`, branches, and options unchanged — only the held
`Model`'s trained state changes.

## Errors

- The `(x, Series y)` / `(x, Thread y)` overloads (including a `Payload`
  routed through them) throw if this `Workflow`'s `Policy` has `filter_nulls`
  set — dropping a row from `x` alone would desynchronize it from the
  already-separate `y`. Use the `(frame, data_options)` overload instead
  (Example 3).
- A `Payload` with `Series` weights (`.weights(sample_weights)`) but a
  frame-backed label-feature path must also name the weight column
  (`.weights(feature_name)`), or `learn_(payload, ...)` throws — otherwise
  `Policy` row-dropping could desynchronize the weights from the frame.
- Every error `Model::learn` itself can raise (missing label column, an
  unhandled `Note` feature, a task/label-shape mismatch) applies unchanged,
  since `Workflow::learn` ends by calling it.

## Practical Examples

### Example 1

Baseline: `Policy` drops a raw ID column before the frame ever reaches the
held `Model`, then `learn_` fits in place.

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
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression(), policy);

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  std::cout << "before: trained = " << std::boolalpha << wf.trained() << "\n";
  wf.learn_(raw, data_options);
  std::cout << "after:  trained = " << wf.trained() << "\n";

  const pluss::Series pred = wf.infer(raw, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
before: trained = false
after:  trained = true
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

The `Payload` overload, supervised by an already-separate label `Series`
rather than a column name. `learn` (not `learn_`) leaves the original
`Workflow` a reusable template.

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
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  const workflow::Payload payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);

  const workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  const workflow::Workflow trained = wf.learn(payload);

  std::cout << "wf.trained()      = " << std::boolalpha << wf.trained()
            << "  (unchanged -- learn(payload) returns a copy)\n";
  std::cout << "trained.trained() = " << trained.trained() << "\n";

  const pluss::Series pred = trained.infer(payload);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
wf.trained()      = false  (unchanged -- learn(payload) returns a copy)
trained.trained() = true
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 3

A `Policy` that can drop rows (`reject_any_missing`, which sets
`filter_nulls`) is rejected on the `(x, y)` overload — the exact failure mode
named in Errors above.

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

  const DataFrame x({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9}, {4}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);

  workflow::Policy policy{};
  policy.reject_any_missing({"income"});

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression(), policy);
  try {
    wf.learn_(x, y);
    std::cout << "learn_(x, y): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "learn_(x, y) threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
learn_(x, y) threw: lymph::workflow::Workflow::learn_(x,y): Policy can drop rows; use the frame + DataFrameBatchOptions overload so features and labels stay aligned.
```

## See Also

- [`models::Model::learn`](../../models/Model/learn.md) — the fitting
  mechanics `Workflow::learn` delegates to.
- [`on`](on.md) — builds the branch/route prep pipeline `learn` fits.
- [`map`](map.md) — preparation only, no estimator fit.
- [`Policy`](../Policy.md) — the admission contract applied first.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
<!-- apiref:nav:end -->
