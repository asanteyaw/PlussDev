# `reports` Introduction

## What This Family Owns

`reports` owns two things that do not fit inside any single other family:

1. **The model-backed evaluation facade.** [`metrics`](../metrics.md) scores a
   truth `Series` against a prediction `Series`. `reports` does the same, but
   its entry points take a trained [`models::Model`](../models/Model.md) or a
   [`workflow::Workflow`](../workflow.md) plus a dataset, run the inference, and
   score the result — so the caller never threads `model.infer(x)` through by
   hand.

2. **The report-type index.** Report objects are defined across `metrics` and
   `selection`, but they all carry a `reports::` alias with a single short
   spelling. The [landing page](../reports.md) tabulates every alias, its
   underlying type, and its home family.

## Facade, Not Reimplementation

Nothing in `reports` computes a metric. `model.reports().classification_summary(
frame)` is exactly:

Train a logistic model, predict on its features, and summarize the predictions against the true labels. The program prints the prediction count and the number of accuracy values in the summary.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const lymph::LabeledBatch batch = fixture_batch(cfg);
  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(batch);
  const pluss::Series y_pred = model.infer(batch.features);
  const lymph::ClassReport summary = lymph::metrics::classification::summary(batch.y, y_pred);
  std::cout << "predictions = " << y_pred.numel() << "\n";
  std::cout << "summary accuracy values = " << summary.accuracy.numel() << "\n";
  return 0;
}
```

Output

```text
predictions = 8
summary accuracy values = 1
```

with the first line folded in. The value is convenience and correctness — the
inference and the scoring stay in step, and the same call works for a `Model`
or a whole `Workflow`.

## Reading Order

1. [`reports::Model`](Model.md) — `model.reports()` and everything it bundles
2. [`classification_summary` / `confusion_table`](classification_summary.md) —
   the model / workflow overloads
3. [`threshold_curve` / `best_threshold`](threshold_curve.md) — operating points

## Related Pages

- [`metrics`](../metrics.md) — the compute layer these calls delegate to.
- [`models::Model`](../models/Model.md) — `model.reports()` is a method on it.
- [`selection`](../selection.md) — home of `reports::Compare` and
  `reports::Tune`.
