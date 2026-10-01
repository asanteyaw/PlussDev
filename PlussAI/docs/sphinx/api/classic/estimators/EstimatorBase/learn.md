<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 68ff48f807dd -->
# `pluss::ai::classic::EstimatorBase::learn`

`learn` fits the estimator to a labeled batch. It is the one method every
concrete classifier and regressor family must implement, and the only method
that changes `trained()` from `false` to `true`.

## Signature

```cpp
classic::Status classic::EstimatorBase::learn(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});
```

## What It Does

Fits this estimator's internal parameters to `batch`, applying whatever
missing-value handling, class weighting, row balancing, and other per-fit
configuration `options` specifies -- family-specific work (a linear solve,
a tree split search, a distance-based fit, whatever the concrete
classifier or regressor implements). It is the only method that moves
`trained()` from `false` to `true`, and every call re-fits from scratch
rather than continuing a previous fit.

## Parameters

`batch` — `LabeledBatch`
: The features, target, and optional sample weights to fit. See
  [`LabeledBatch`](../../data/LabeledBatch.md).

`options` — `TrainOptions`, default `{}`
: Per-fit configuration — missing-value handling, class weighting,
  calibration, row balancing, solver limits. See
  [`TrainOptions`](../../core/TrainOptions.md) for every field.

## Returns

A `Status`. `.ok()` on success; `trained()` is `true` immediately afterward.
On failure, `trained()`'s value is unspecified by this contract — do not
assume a failed `learn` leaves a previous fit intact; check `.ok()` before
calling `infer` / `assess`.

## Errors

- `Status::shape_mismatch` — a batch shape problem (`validate_labeled` would
  have caught it): undefined `y`, a length mismatch between `y` and the
  sample count, and so on.
- `Status::invalid_argument` — a family-specific configuration problem, most
  notably a `Linear` classifier whose `LinearFamilyOptions::objective` is not
  `Logistic` or `Hinge` — see [`LinearFamilyOptions`](../../families/LinearFamilyOptions.md#objective).
- Every fitting call re-fits from scratch: `learn` on an already-trained
  estimator discards the previous fit and starts over (`TrainOptions::warm_start`
  is documented to change this but is not currently consulted by any fit
  kernel — see [`TrainOptions`](../../core/TrainOptions.md#warm_start)).

## Practical Examples

### Example 1

Fitting a classifier, and reading `trained()` before and after.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  std::cout << "trained before learn = " << std::boolalpha << made.value()->trained() << "\n";
  const cls::Status learned = made.value()->learn(batch);
  std::cout << "learn ok = " << learned.ok() << "\n";
  std::cout << "trained after learn  = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
trained before learn = false
learn ok = true
trained after learn  = true
```

### Example 2

Fitting a regressor with an explicit `TrainOptions`, and re-fitting the same
estimator a second time (a fresh fit, not a warm-started refinement).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.1, 3.9, 6.1, 7.9}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::TrainOptions opts{};
  opts.max_iterations = 500;
  const cls::Status first = made.value()->learn(batch, opts);
  std::cout << "first learn ok  = " << std::boolalpha << first.ok() << "\n";

  const cls::Status second = made.value()->learn(batch, opts);  // re-fits from scratch
  std::cout << "second learn ok = " << second.ok() << "\n";

  return 0;
}
```

Output

```text
first learn ok  = true
second learn ok = true
```

### Example 3

A shape-mismatched batch — `learn` fails and reports why instead of fitting
garbage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);  // 3 labels, 4 samples

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  const cls::Status learned = made.value()->learn(batch);
  std::cout << "ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "message = \"" << learned.message << "\"\n";
  std::cout << "trained() after the failed fit = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
ok = false
message = "LabeledBatch.y length must match sample count."
trained() after the failed fit = false
```

## See Also

- [`infer`](infer.md) / [`assess`](assess.md) — what you can call once
  `learn` succeeds.
- [`TrainOptions`](../../core/TrainOptions.md) — every configuration field.
- [`LabeledBatch`](../../data/LabeledBatch.md) — the input shape.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="load_state.html">
    <span class="label">Next</span>
    <span class="title">load_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
