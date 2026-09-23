# `pluss::ai::classic::SparseLinearRegressor`

A linear regressor fit directly on a sparse matrix by full-batch gradient
descent — the regression counterpart to
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md), sharing
the same shape (no constructor options, a fresh
[`LinearFamilyOptions`](../families/LinearFamilyOptions.md) per `learn`
call, a `SparseTripletMatrix`/`SparseCsrMatrix` overload pair on every
method) without the binary-label bookkeeping.

## Signature

```cpp
classic::SparseLinearRegressor();

bool classic::SparseLinearRegressor::trained() const noexcept;
std::int64_t classic::SparseLinearRegressor::feature_count() const noexcept;

classic::Status classic::SparseLinearRegressor::learn(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});
classic::Status classic::SparseLinearRegressor::learn(
    const classic::SparseCsrMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});

classic::StatusOr<pluss::Series> classic::SparseLinearRegressor::infer(
    const classic::SparseTripletMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearRegressor::infer(
    const classic::SparseCsrMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearRegressor::assess(
    const classic::SparseTripletMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::SparseLinearRegressor::assess(
    const classic::SparseCsrMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;

classic::StatusOr<std::vector<pluss::Series>> classic::SparseLinearRegressor::dump_state() const;
classic::Status classic::SparseLinearRegressor::load_state(
    /* a range of pluss::Series -- pass a std::vector<Series> or braced list */ state);
```

## Construction

`SparseLinearRegressor()` takes no arguments; `LinearFamilyOptions` is
supplied fresh per `learn` call, and again only `regularization`/
`regularization_strength` are actually read:

- `objective` — **read but ignored, always.** This class only ever fits
  plain least squares — **`LinearObjective::RobustLeast` is not
  available here at all**, unlike the batch `Linear` family and
  [`OnlineLinearRegressor`](../online/OnlineLinearRegressor.md), both of
  which support it. See [Example 3](#example-3).
- `fit_intercept` — ignored; a bias term is always fit. See
  [Example 4](#example-4).
- `elastic_l1_ratio` / `robust_transition` — never read.

## State

`trained()` — `bool` — `true` once `learn` has succeeded.

`feature_count()` — `std::int64_t` — the column count `learn` (or
`load_state`) established.

## Methods

`learn(x, y, options = {}, linear_options = {})`
: Full-batch gradient descent at a fixed learning rate of `0.05`, up to
  `options.max_iterations` steps, stopping early once successive losses
  differ by no more than `options.tolerance`. `options.max_iterations`
  and `options.tolerance` must both be `> 0`. `y` may hold any finite
  values — there is no class-count requirement the way
  [`SparseLinearBinaryClassifier::learn`](SparseLinearBinaryClassifier.md#methods)
  has. Resets all fitted state on every call.

`infer(x)`
: The raw linear prediction (`bias + weights · x`) — no threshold, no
  transform.

`assess(x, y, options = {})`
: `options.metrics` empty (the default) requests `{Mse, Rmse, R2}`.
  Supports `Mse`, `Rmse`, `Mae`, `R2` only — every other `MetricKind` is
  rejected with `Status::not_implemented`.

`dump_state()` / `load_state(state)`
: `[metadata, weights]` — the same shape as
  [`SparseLinearBinaryClassifier`'s own](SparseLinearBinaryClassifier.md#methods).

## Errors

- `learn`: `options.max_iterations <= 0` or `options.tolerance <= 0` —
  `Status::invalid_argument`.
- `learn`: `y` undefined, its length not matching the row count, or
  non-finite — `Status::invalid_argument` / `Status::shape_mismatch`.
- `learn`: the fit diverges — `Status::numerical_failure`.
- `infer` / `assess`: called before a successful `learn` —
  `Status::precondition`.
- `infer` / `assess`: the matrix's column count does not match
  `feature_count()` — `Status::shape_mismatch`.
- `assess`: `y` undefined, or its length not matching the row count —
  `Status::invalid_argument` / `Status::shape_mismatch`.
- `assess`: a requested `MetricKind` this class does not support —
  `Status::not_implemented`.
- `load_state`: fewer than 2 tensors, or a coefficient-vector length
  mismatch — `Status::invalid_argument` / `Status::shape_mismatch`.

## Practical Examples

The full lifecycle, the `SparseCsrMatrix` overload proven identical to
`SparseTripletMatrix`, `objective = RobustLeast` proven to have zero
effect (unlike the batch and online regressors), `fit_intercept` also
proven to have zero effect, L2 regularization shrinking the fit, an error
roundup, and a `dump_state`/`load_state` round trip.

### Example 1

Four rows, one feature, `y` an exact linear function of it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor model;
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "trained()       = " << model.trained() << "\n";
  std::cout << "feature_count() = " << model.feature_count() << "\n";
  std::cout << "infer = " << model.infer(x).value() << "\n";
  std::cout << "assess (default [Mse, Rmse, R2]) = " << model.assess(x, y).value() << "\n";

  return 0;
}
```

Output

```text
learn ok = true
trained()       = true
feature_count() = 1
infer = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0183495, 4.0088912, 5.999433, 7.9899747]
assess (default [Mse, Rmse, R2]) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.00012914635, 0.011364258, 0.99997417]
```

### Example 2

The `SparseCsrMatrix` overload, proven identical to the
`SparseTripletMatrix` form.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(x).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor via_triplet;
  via_triplet.learn(x, y);
  cls::SparseLinearRegressor via_csr;
  via_csr.learn(csr, y);

  const double diff = (via_triplet.infer(x).value() - via_csr.infer(csr).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff between the two forms = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between the two forms = 0
```

### Example 3

`objective = RobustLeast` has no effect at all — unlike the batch
`Linear` family and `OnlineLinearRegressor`, this class has no robust
fitting path; the result is identical to the default plain least squares.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor plain;
  plain.learn(x, y);

  cls::LinearFamilyOptions robust{};
  robust.objective = cls::LinearObjective::RobustLeast;
  robust.robust_transition = 1.0;
  cls::SparseLinearRegressor odd_objective;
  odd_objective.learn(x, y, {}, robust);

  const double diff = (plain.infer(x).value() - odd_objective.infer(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (objective ignored) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (objective ignored) = 0
```

### Example 4

`fit_intercept = false` also has no effect.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor with_default;
  with_default.learn(x, y);

  cls::LinearFamilyOptions no_intercept{};
  no_intercept.fit_intercept = false;
  cls::SparseLinearRegressor without_intercept_flag;
  without_intercept_flag.learn(x, y, {}, no_intercept);

  const double diff = (with_default.infer(x).value() - without_intercept_flag.infer(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (fit_intercept ignored) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (fit_intercept ignored) = 0
```

### Example 5

`regularization = L2` shrinks the fit toward zero.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor none;
  none.learn(x, y);

  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  cls::SparseLinearRegressor regularized;
  regularized.learn(x, y, {}, l2);

  std::cout << "infer, no penalty = " << none.infer(x).value() << "\n";
  std::cout << "infer, L2 penalty = " << regularized.infer(x).value() << "\n";

  return 0;
}
```

Output

```text
infer, no penalty = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0183495, 4.0088912, 5.999433, 7.9899747]
infer, L2 penalty = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.3331673, 4.4443505, 5.5555337, 6.6667169]
```

### Example 6

An error roundup: `max_iterations <= 0`, `tolerance <= 0`, and an `infer`
feature-count mismatch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::TrainOptions zero_iters{};
  zero_iters.max_iterations = 0;
  cls::SparseLinearRegressor a;
  std::cout << "max_iterations=0 ok = " << std::boolalpha << a.learn(x, y, zero_iters).ok() << "\n";

  cls::TrainOptions zero_tol{};
  zero_tol.tolerance = 0.0;
  cls::SparseLinearRegressor b;
  std::cout << "tolerance=0 ok       = " << b.learn(x, y, zero_tol).ok() << "\n";

  cls::SparseLinearRegressor trained;
  trained.learn(x, y);
  const cls::SparseTripletMatrix wrong_cols = cls::make_sparse_triplet(1, 5, {0}, {4}, {1.0}).value();
  std::cout << "feature-count mismatch ok = " << trained.infer(wrong_cols).ok() << "\n";

  return 0;
}
```

Output

```text
max_iterations=0 ok = false
tolerance=0 ok       = false
feature-count mismatch ok = false
```

### Example 7

`dump_state` / `load_state` round trip.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 1, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.0, 2.0, 3.0, 4.0}).value();
  const pluss::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearRegressor original;
  original.learn(x, y);
  const std::vector<pluss::Series> state = original.dump_state().value();

  cls::SparseLinearRegressor restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.infer(x).value() - restored.infer(x).value())
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff after restore = " << diff << "\n";

  return 0;
}
```

Output

```text
load_state ok = true
max abs diff after restore = 0
```

## Changelog

- **0.1** — constructor, `trained`/`feature_count`, `learn`/`infer`/
  `assess` (`SparseTripletMatrix` and `SparseCsrMatrix` overloads),
  `dump_state`/`load_state`.

## See Also

- [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
  [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — the
  classification counterparts.
- [`cross_assess_sparse`](cross_assess_sparse.md) — cross-validated
  scoring built on this class.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
