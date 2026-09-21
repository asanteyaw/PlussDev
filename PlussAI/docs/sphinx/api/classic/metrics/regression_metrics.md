# `pluss::ai::classic::regression_metrics`

Compute Mse/Rmse/Mae/R2 from truth and prediction vectors you already
hold — the regression counterpart to
[`classification_metrics`](classification_metrics.md), for predictions that
did not come directly from a `RegressorBase::infer` call (for instance,
after inverse-transforming predictions back out of a rescaled label space).

## Signature

```cpp
classic::StatusOr<pluss::Series> classic::regression_metrics(
    const pluss::Series& truth,
    const pluss::Series& prediction,
    const std::vector<classic::MetricKind>& metrics = {});
```

## Parameters

`truth` / `prediction`
: Rank-any `Series` (reshaped to rank-1 internally), equal length, at least
  one sample. Both must be defined.

`metrics`
: Which `MetricKind`s to compute, in order. Empty (the default) requests
  `{Mse, Rmse, R2}` — the same default every regression `assess` in this
  reference uses. Only `Mse`, `Rmse`, `Mae`, `R2` are supported; anything
  else is rejected.

## Returns

A `StatusOr<Series>`, `Float64`, length `metrics.size()` (or `3` for the
default list), one value per requested metric in request order:

- `Mse` — mean squared error, `mean((prediction - truth)^2)`.
- `Rmse` — `sqrt(Mse)`.
- `Mae` — mean absolute error, `mean(|prediction - truth|)`.
- `R2` — the coefficient of determination,
  `1 - sum((prediction - truth)^2) / sum((truth - mean(truth))^2)`. `0.0`
  when `truth`'s own variance is at or below machine epsilon (a constant or
  near-constant truth vector, where R2 is otherwise undefined) — this
  fallback triggers on `truth`'s variance alone, never on `prediction`'s.

## Errors

- `truth` or `prediction` undefined — `Status::invalid_argument`.
- Length mismatch — `Status::shape_mismatch`.
- Zero-length input — `Status::invalid_argument`.
- A requested `MetricKind` outside `{Mse, Rmse, Mae, R2}` —
  `Status::invalid_argument`.

## Practical Examples

The default metric list on an exact-fit case (R2 = 1), a reordered explicit
subset, and the constant-truth fallback.

### Example 1

Predictions matching truth exactly: `Mse`/`Rmse`/`Mae` all `0`, `R2 = 1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);

  const pluss::Series values = cls::regression_metrics(truth, pred).value();
  std::cout << "[Mse, Rmse, R2] = " << values << "\n";

  return 0;
}
```

Output

```text
[Mse, Rmse, R2] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 1]
```

### Example 2

An imperfect fit, requesting `Mae` before `Mse`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({1.0, 3.0, 3.0, 6.0}, {4}, f64);

  const pluss::Series values =
      cls::regression_metrics(truth, pred, {cls::MetricKind::Mae, cls::MetricKind::Mse}).value();
  std::cout << "[Mae, Mse] = " << values << "\n";

  return 0;
}
```

Output

```text
[Mae, Mse] = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.75, 1.25]
```

### Example 3

A constant truth vector: `R2` falls back to `0.0` instead of dividing by a
zero total sum of squares.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({5.0, 5.0, 5.0}, {3}, f64);
  const pluss::Series pred = pluss::make_series({4.0, 5.0, 6.0}, {3}, f64);

  const pluss::Series values = cls::regression_metrics(truth, pred, {cls::MetricKind::R2}).value();
  std::cout << "R2 (constant truth) = " << values << "\n";

  return 0;
}
```

Output

```text
R2 (constant truth) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0]
```

## Changelog

- **0.1** — `Mse`/`Rmse`/`Mae`/`R2`.

## See Also

- [`classification_metrics`](classification_metrics.md) — the
  classification counterpart.
- [`EstimatorBase::assess`](../estimators/EstimatorBase/assess.md) — the
  model-attached path with the same default metric list.
