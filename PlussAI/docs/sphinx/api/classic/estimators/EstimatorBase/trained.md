<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c198e893e22a -->
# `pluss::ai::classic::EstimatorBase::trained`

`trained` reports whether `learn` has succeeded at least once. It is the
cheapest possible check before calling `infer` / `assess` (both fail cleanly
on an untrained estimator regardless, but `trained()` lets you branch without
constructing a batch first).

## Signature

```cpp
bool classic::EstimatorBase::trained() const noexcept;
```

## What It Does

Reports whether [`learn`](learn.md) has succeeded at least once on this
estimator -- a plain flag check, not a re-derivation from internal state.
It exists as the cheapest possible way to branch on trained-vs-untrained
before calling [`infer`](infer.md)/[`assess`](assess.md), without first
having to construct a batch just to trigger their own precondition check.

## Parameters

None.

## Returns

`false` immediately after construction (from `make_classifier` /
`make_regressor`, before any `learn` call). `true` once a `learn` call has
returned an ok `Status`. A `learn` call that fails does not necessarily leave
`trained()` at its previous value — treat a failed `learn`'s effect on
`trained()` as unspecified and re-check after every `learn` call rather than
assuming.

## Errors

None — `noexcept`, always succeeds.

## Practical Examples

### Example 1

`trained()` before and after a successful `learn`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  std::cout << "trained before = " << std::boolalpha << made.value()->trained() << "\n";

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  made.value()->learn(batch);

  std::cout << "trained after  = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
trained before = false
trained after  = true
```

### Example 2

Using `trained()` to guard a call that would otherwise fail — check first
rather than reading the error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});

  if (made.value()->trained()) {
    std::cout << "prediction = " << made.value()->infer(probe).value() << "\n";
  } else {
    std::cout << "skipped infer: estimator is not trained yet\n";
  }

  return 0;
}
```

Output

```text
skipped infer: estimator is not trained yet
```

### Example 3

Two independent estimators built from the same `ModelSpec` — each tracks its
own `trained()` state; fitting one does not affect the other.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> a = cls::make_regressor(spec);
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> b = cls::make_regressor(spec);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  a.value()->learn(batch);  // only fit "a"

  std::cout << "a.trained() = " << std::boolalpha << a.value()->trained() << "\n";
  std::cout << "b.trained() = " << b.value()->trained() << "\n";

  return 0;
}
```

Output

```text
a.trained() = true
b.trained() = false
```

## See Also

- [`learn`](learn.md) — the only method that can move `trained()` to `true`.
- [`task_kind`](task_kind.md) — the other fixed-vs-mutable state query.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="task_kind.html">
    <span class="label">Previous</span>
    <span class="title">task_kind</span>
  </a>
</div>
<!-- apiref:nav:end -->
