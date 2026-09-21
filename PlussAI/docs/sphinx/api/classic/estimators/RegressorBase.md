# `pluss::ai::classic::RegressorBase`

`RegressorBase` is [`EstimatorBase`](EstimatorBase.md) fixed to
`TaskKind::Regression`. Unlike [`ClassifierBase`](ClassifierBase.md), it adds
no methods of its own — a regressor's prediction *is* its raw output, so
there is no separate "scores versus hard label" distinction to expose.
[`make_regressor`](make_regressor.md) is the only way to obtain one.

## Signature

```cpp
// Everything EstimatorBase declares (learn, infer, assess, task_kind,
// trained, dump_state, load_state) applies unchanged -- see EstimatorBase.
// task_kind() is fixed:
classic::TaskKind classic::RegressorBase::task_kind() const noexcept;  // always Regression
```

## Construction

Obtained only from [`make_regressor`](make_regressor.md), which requires a
`ModelSpec` with `.task == TaskKind::Regression`. You hold it as
`std::unique_ptr<RegressorBase>` (or a reference/pointer to that).

## State

Everything [`EstimatorBase`](EstimatorBase.md#state) exposes, unchanged.
`task_kind()` always returns `Regression` regardless of which concrete family
implements it.

## Methods

Every [`EstimatorBase`](EstimatorBase.md#methods) method — `learn`, `infer`,
`assess`, `task_kind`, `trained`, `dump_state`, `load_state` — applies
unchanged and is documented in full on that page. `RegressorBase` adds
nothing beyond fixing `task_kind()`.

## Practical Example

A minimal regressor lifecycle — the same five calls `EstimatorBase`'s own
pages document individually, on a `RegressorBase` specifically.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  std::cout << "task_kind() == Regression: " << std::boolalpha
            << (made.value()->task_kind() == cls::TaskKind::Regression) << "\n";
  std::cout << "trained before learn      : " << made.value()->trained() << "\n";
  made.value()->learn(batch);
  std::cout << "trained after learn       : " << made.value()->trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "prediction for x = 5      : " << made.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
```

Output

```text
task_kind() == Regression: true
trained before learn      : false
trained after learn       : true
prediction for x = 5      : 10
```

## Changelog

- **0.1** — the `EstimatorBase` contract fixed to `Regression`, no additional
  methods.

## See Also

- [`EstimatorBase`](EstimatorBase.md) — every inherited method, documented in
  full.
- [`ClassifierBase`](ClassifierBase.md) — the classification counterpart,
  which adds `infer_scores`.
- [`make_regressor`](make_regressor.md) — how to obtain one.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
