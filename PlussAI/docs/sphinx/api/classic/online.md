# `pluss::ai::classic::online`

`online` owns incremental, gradient-descent-fit linear estimators —
[`OnlineLinearClassifier`](online/OnlineLinearClassifier.md) and
[`OnlineLinearRegressor`](online/OnlineLinearRegressor.md) — for the case
the rest of this reference does not cover: keeping a model up to date as
new data arrives, one `advance` step at a time, without ever re-fitting
from scratch. It is a small, self-contained module — it does not
participate in [`estimators`](estimators.md)/[`compose`](compose.md)/
[`tune`](tune.md)/[`io`](io.md) the way every other family here does; see
[`Introduction`](online/Introduction.md) for exactly what that means.

## The Surface

| Type / function | What it gives you | Reach for it when |
| --- | --- | --- |
| [`OnlineLinearClassifier`](online/OnlineLinearClassifier.md) | a binary `Logistic`/`Hinge` classifier, fit incrementally | you need to keep a classifier current as labeled batches keep arriving |
| [`OnlineLinearRegressor`](online/OnlineLinearRegressor.md) | a `LeastSquares`/`RobustLeast` regressor, fit incrementally | the same, for a continuous target |
| [`make_online_linear_classifier`](online/make_online_linear_classifier.md) / [`make_online_linear_regressor`](online/make_online_linear_regressor.md) | the eager-validation constructors | you want a bad `OnlineLinearOptions` rejected immediately, not deferred to the first `learn` call |
| [`OnlineLinearOptions`](online/OnlineLinearOptions.md) | which objective, how regularized, the learning-rate schedule | configuring either class |

## How To Call It

Start an online linear classifier on a labeled batch, advance it with another batch, then predict on the features. The program prints the prediction count.

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
  const cls::LabeledBatch first = fixture_batch(cfg);
  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model =
      cls::online::make_online_linear_classifier(options).value();
  model.learn(first);
  model.advance(first);
  const cls::StatusOr<pluss::Series> predicted = model.infer(first.features);
  std::cout << "predictions = " << predicted.value().numel() << "\n";
  return 0;
}
```

Output

```text
predictions = 8
```

## Related Pages

- [`families`](families.md) — `LinearFamilyOptions`, embedded in
  `OnlineLinearOptions`.
- [`core`](core.md) — `TrainOptions`, including `max_iterations` (the
  hidden bootstrap-step count inside `learn`).
- [`metrics`](metrics.md) — the same metric formulas `assess` computes
  here.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

online/Introduction
online/OnlineLinearClassifier
online/OnlineLinearRegressor
online/make_online_linear_classifier
online/make_online_linear_regressor
online/OnlineLinearOptions
```
<!-- apiref:toctree:end -->
