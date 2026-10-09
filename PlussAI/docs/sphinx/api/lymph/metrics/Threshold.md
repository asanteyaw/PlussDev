<!-- pluss-hand-authored -->

# `lymph::metrics::classification::Threshold`

[Back to `metrics`](../metrics.md)

`Threshold` is the **operating-point accessor**. Built from a score `Series` and
a label `Series` by `metrics::classification::threshold(scores, labels)`, it answers two
questions: what does precision / recall / f1 look like across *every* achievable
cutoff (`curve()`), and which single cutoff maximizes a chosen objective
(`best()`).

## Scope And Alias

The type is `lymph::metrics::classification::Threshold`. There is no shorter alias — it is a
compute accessor, not a report. `scores()` and `labels()` read the cached
inputs back.

## Construction

```cpp
lymph::metrics::classification::Threshold lymph::metrics::classification::threshold(
    pluss::Series scores,
    pluss::Series labels);
```

`scores` are the model's raw positive-class scores (e.g. from
`model.infer_scores(...)`); `labels` are the matching ground-truth 0/1 labels.
Nothing is computed at construction — each method call runs the sweep.

## The `ThresholdCurve` type

`curve()` returns a `ThresholdCurve` — four aligned `Series`:

`Series thresholds`
: Every achievable decision threshold, **descending**.

`Series precision` · `recall` · `f1`
: The metric at each corresponding threshold.

Index *i* across all four is one operating point. The sweep is a single
`O(n log n)` pass (one sort, cumulative true / false positive counts).

## Methods

### `curve`

```cpp
ThresholdCurve curve() const;
```

The full precision / recall / f1 sweep.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(frame, data_options);
  const pluss::Series scores = model.infer_scores(frame, data_options);

  const lymph::ThresholdCurve curve =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series()).curve();

  std::cout << "points     = " << curve.thresholds.numel() << "\n";
  std::cout << "thresholds = " << curve.thresholds.slice(0, 0, 4) << "\n";
  std::cout << "precision  = " << curve.precision.slice(0, 0, 4) << "\n";
  std::cout << "recall     = " << curve.recall.slice(0, 0, 4) << "\n";
  std::cout << "f1         = " << curve.f1.slice(0, 0, 4) << "\n";

  return 0;
}
```

Output

```text
points     = 16
thresholds = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[4.1479996, 3.594933, 3.0418664, 2.4887998]
precision  = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
recall     = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.125, 0.25, 0.375, 0.5]
f1         = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.22222222, 0.4, 0.54545455, 0.66666667]
```

### `best`

```cpp
double best(MetricKind objective = MetricKind::F1) const;
```

The threshold that maximizes `objective` over the sweep. `Precision`, `Recall` /
`Sensitivity`, and `F1` are supported; any other `MetricKind` falls back to
`F1`.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(frame, data_options);
  const pluss::Series scores = model.infer_scores(frame, data_options);

  const lymph::metrics::classification::Threshold th =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series());

  std::cout << "best for F1        = " << th.best(lymph::MetricKind::F1) << "\n";
  std::cout << "best for Precision = " << th.best(lymph::MetricKind::Precision) << "\n";
  std::cout << "best for Recall    = " << th.best(lymph::MetricKind::Recall) << "\n";

  return 0;
}
```

Output

```text
best for F1        = 0.276533
best for Precision = 4.148
best for Recall    = -1.38267
```

### Using `best()` for inference

Feed the chosen threshold back through
[`InferOptions`](../models/Model/infer.md)`.positive_threshold` so labelling
happens at that cutoff instead of the default `0.5`.

#### Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(frame, data_options);
  const pluss::Series scores = model.infer_scores(frame, data_options);

  const double tuned =
      lymph::metrics::classification::threshold(scores, frame.features("target").as_series())
          .best(lymph::MetricKind::F1);
  std::cout << "tuned threshold = " << tuned << "\n";

  lymph::InferOptions tuned_infer{};
  tuned_infer.positive_threshold = tuned;

  const pluss::Series at_default = model.infer(frame.features({"x"}));
  const pluss::Series at_tuned = model.infer(frame.features({"x"}), tuned_infer);
  std::cout << "labels @ 0.5   = " << at_default << "\n";
  std::cout << "labels @ tuned = " << at_tuned << "\n";

  return 0;
}
```

Output

```text
tuned threshold = 0.276533
labels @ 0.5   = pluss::Series(shape=[16], strides=[1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1]
labels @ tuned = pluss::Series(shape=[16], strides=[1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
```

## Changelog

- **0.1** — `metrics::classification::threshold(scores, labels)` → `curve()` (full
  `ThresholdCurve` sweep) and `best(objective)`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
