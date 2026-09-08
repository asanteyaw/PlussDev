<!-- pluss-hand-authored -->

# `lymph::reports::ModelReports`

[Back to `reports`](../reports.md)

`ModelReports` — aliased `reports::Model`, and `lymph::ModelReports` — is the
accessor returned by [`models::Model::reports()`](../models/Model.md). It
binds a trained model once and exposes every model-backed evaluation helper
as a method, so you can score that one model against several datasets without
re-passing it each time.

## Scope And Alias

`lymph::reports::ModelReports`, with `reports::Model` and `lymph::ModelReports`
as aliases. It is a small value type holding a copy of the `Model`; construct
it only through `model.reports()`.

## Construction

```cpp
reports::ModelReports models::Model::reports() const;
```

`model.reports()` copies the model into the accessor (`Model` is a cheap
copy-on-write handle). The accessor stays usable for the life of the
returned value; a later `learn_` on the original model does not change what
the accessor holds.

## State

`const models::Model& model() const`
: The bound model. Reach in for a `Model` method the accessor does not wrap.

## Methods

Every method is the free function of the same name from
[`reports`](../reports.md), with the `model` argument bound. Same overloads
(the `(x, y)`, `(frame, data_options)`, and `LabeledBatch` data shapes), same
returns.

| Method | Returns | Covered on |
|--------|---------|-----------|
| `confusion_table(data)` | `ConfusionTable` | [`classification_summary`](classification_summary.md) |
| `classification_summary(data)` | [`ClassReport`](../metrics/ClassReport.md) | [`classification_summary`](classification_summary.md) |
| `metric_series(data)` | `Series` of scalar metrics | [`classification_summary`](classification_summary.md) |
| `threshold_curve(data)` | `ThresholdCurve` | [`threshold_curve`](threshold_curve.md) |
| `best_threshold(data, objective = MetricKind::F1)` | `double` | [`threshold_curve`](threshold_curve.md) |

## Practical Example

One `model.reports()` accessor, several helpers.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Thread(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::reports::Model r = model.reports();

  const lymph::ClassReport summary = r.classification_summary(x, y);
  std::cout << "accuracy       = " << summary.accuracy.item<double>() << "\n";

  const lymph::ConfusionTable table = r.confusion_table(x, y);
  std::cout << "confusion counts =\n" << table.counts << "\n";

  const lymph::ThresholdCurve curve = r.threshold_curve(x, y);
  std::cout << "threshold count = " << curve.thresholds.numel() << "\n";

  std::cout << "best_threshold(F1) = " << r.best_threshold(x, y, lymph::MetricKind::F1) << "\n";
  std::cout << "r.model().trained() = " << std::boolalpha << r.model().trained() << "\n";

  return 0;
}
```

Output

```text
accuracy       = 1
confusion counts =
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[5, 0],
  [0, 5]]
threshold count = 8
best_threshold(F1) = 0.999406
r.model().trained() = true
```

## Changelog

- **0.1** — `model.reports()` accessor bundling `confusion_table` /
  `classification_summary` / `metric_series` / `threshold_curve` /
  `best_threshold`.

## See Also

- [`classification_summary`](classification_summary.md) /
  [`threshold_curve`](threshold_curve.md) — the free-function forms and full
  parameter detail.
- [`models::Model`](../models/Model.md) — `reports()` is a method on it.
- [`metrics`](../metrics.md) — the compute layer everything here delegates
  to.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
