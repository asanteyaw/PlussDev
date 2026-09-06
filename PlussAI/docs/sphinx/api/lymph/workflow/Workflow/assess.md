<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a6987edcd8a1 -->

# `lymph::workflow::Workflow::assess`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
MetricReport Workflow::assess(const LabeledBatch& batch, const AssessOptions& options = {}) const;
MetricReport Workflow::assess(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const AssessOptions& options = {}) const;
MetricReport Workflow::assess(
    const pluss::table::DataFrame& x, const Series& y, const AssessOptions& options = {}) const;
MetricReport Workflow::assess(
    const pluss::table::DataFrame& x, const pluss::table::Thread& y, const AssessOptions& options = {}) const;
MetricReport Workflow::assess(const Payload& payload, const AssessOptions& options = {}) const;
```

## What `Workflow` Adds

`assess` on a `Workflow` is `Policy::apply(...)` (on any argument that
carries a raw frame) followed by the identically-shaped call on the held
`Model`. `assess` is [`infer`](infer.md) plus a scoring step — see
[`models::Model::assess`](../../models/Model/assess.md) for how the
`MetricReport` is built and populated, and its "The `MetricReport`" section
for the "only populated if requested" rule.

## Input Shapes

Four of the five shapes are identical to `Model::assess`'s — see
[Input Shapes there](../../models/Model/assess.md#input-shapes). Named here:
`batch` (a pre-assembled `LabeledBatch`, skips `Policy`), `raw_frame` with
`data_options`, and `x`/`y` (a `Series` or `Thread` label). The fifth is
`Workflow`-only:

`payload`
: A [`Payload`](../Payload.md). Routes through the `raw_frame`/`data_options`
  path if it holds a named label column, or the `x`/`y` path if it holds a
  `Series` label — same routing `learn(payload)` uses, and subject to the
  same `filter_nulls`-safety restriction on the `x`/`y` path (Errors).

`options` (`lymph::options::Assess`) is unchanged from `Model::assess` — see
[its Parameters section](../../models/Model/assess.md#parameters) for the
full field list (`metrics`, `positive_threshold`, `regression_quantile`,
`sample_weights`, `layout`).

## Returns

A `MetricReport`, exactly as `Model::assess` returns — one field per
requested metric, each a one-element `Series`.

## Errors

- The `(x, Series y)` / `(x, Thread y)` overloads (including a `Payload`
  routed through them) throw if this `Workflow`'s `Policy` has `filter_nulls`
  set — the same restriction [`learn`](learn.md#errors) has, for the same
  reason: dropping a row from `x` alone would desynchronize it from the
  already-separate `y` (Example 3). Use the `(raw_frame, data_options)`
  overload instead.
- Every error `Model::assess` itself can raise (an unrequested-but-read
  `MetricReport` field, a label-shape mismatch) applies unchanged.

## Practical Examples

### Example 1

Baseline: `assess` on the training frame itself, reading two requested
metrics off the returned `MetricReport`.

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
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(raw, data_options);

  lymph::options::Assess assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::MetricReport report = trained.assess(raw, data_options, assess_options);
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "f1       = " << report.f1.item<double>() << "\n";

  return 0;
}
```

Output

```text
accuracy = 1
f1       = 1
```

### Example 2

`assess(payload)`, with the `Payload` supervised by a `Series` label.

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
  const workflow::Workflow trained = workflow::make(lymph::models::logistic_regression()).learn(payload);

  lymph::options::Assess assess_options{};
  assess_options.metrics = {lymph::MetricKind::Accuracy};

  const lymph::MetricReport report = trained.assess(payload, assess_options);
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";

  return 0;
}
```

Output

```text
accuracy = 1
```

### Example 3

A `Policy` that can drop rows is rejected on `assess(x, y)` — the same
restriction `learn_(x, y)` has, and for the same reason.

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

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(x, y);
  const workflow::Workflow with_policy = workflow::make(trained.model(), policy);

  try {
    (void)with_policy.assess(x, y);
    std::cout << "assess(x, y): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "assess(x, y) threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
assess(x, y) threw: lymph::workflow::Workflow::assess(x,y): Policy can drop rows; use the frame + DataFrameBatchOptions overload so features and labels stay aligned.
```

## See Also

- [`models::Model::assess`](../../models/Model/assess.md) — the scoring
  mechanics `Workflow::assess` delegates to, and the `MetricReport` contract.
- [`infer`](infer.md) — the prediction step `assess` scores.
- [`selection`](../../selection.md) — resampling-based model comparison, not
  a single-call score on one fixed set of rows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
