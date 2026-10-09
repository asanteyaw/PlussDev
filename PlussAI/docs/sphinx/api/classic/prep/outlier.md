# `pluss::ai::classic::prep::outlier`

`outlier` owns scoring and flagging anomalous rows — a different shape of
problem from every other `prep` sub-family, which all *reshape column
values*. `outlier` instead asks a per-row question ("how unusual is this
row, relative to the training data as a whole?") and answers it with a
score or a label, not a transformed feature matrix.

Today this is one type: [`OutlierIsolator`](outlier/OutlierIsolator.md), an
isolation-forest anomaly detector. It is **not** a `TransformerBase` — its
public shape is `learn` / `score_samples` / `infer` / `learn_infer` rather
than `learn` / `map` / `learn_map`, reflecting that its output is a score or
a label, not a re-expression of the input features.

## How To Call It

Fit an outlier isolator on one feature with a single extreme value, then score and label each row. The program prints how many scores and labels came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.1, 0.0, 0.2, 0.1, 0.0, 9.0, 0.1}, {10, 1}, cfg);
  cls::FeatureBatch batch{};
  batch.x = x;
  cls::prep::OutlierIsolator isolator;
  isolator.learn(batch);
  const pluss::Series scores = isolator.score_samples(batch).value();
  const pluss::Series labels = isolator.infer(batch).value();
  std::cout << "scores = " << scores.numel() << "\n";
  std::cout << "labels = " << labels.numel() << "\n";
  return 0;
}
```

Output

```text
scores = 10
labels = 10
```

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`scaling`](scaling.md) — `RobustScaler`, a *de-emphasizing* alternative
  to outliers rather than flagging them outright.
- [`families`](../families.md) — `DetectFamilyOptions`, the `classic`
  estimator-shaped path to anomaly detection (via `make_transformer`),
  distinct from this standalone type.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

outlier/Introduction
outlier/OutlierIsolator
```
<!-- apiref:toctree:end -->
