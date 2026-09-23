# `pluss::ai::classic::SparseLinearBinaryClassifier`

A binary logistic classifier fit directly on a sparse matrix by full-batch
gradient descent — every `learn` call sees every row on every iteration
(no per-row stochasticity, no learning-rate schedule; contrast
[`online::OnlineLinearClassifier`](../online/OnlineLinearClassifier.md)'s
incremental, decaying-rate fit). Unlike every batch estimator under
[`estimators`](../estimators.md), it takes no `ModelSpec` and is
constructed with no options at all — a
[`LinearFamilyOptions`](../families/LinearFamilyOptions.md) is supplied
fresh to each `learn` call instead of being fixed at construction.

## Signature

```cpp
classic::SparseLinearBinaryClassifier();  // no options -- see learn()

bool classic::SparseLinearBinaryClassifier::trained() const noexcept;
std::int64_t classic::SparseLinearBinaryClassifier::feature_count() const noexcept;

classic::Status classic::SparseLinearBinaryClassifier::learn(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});
classic::Status classic::SparseLinearBinaryClassifier::learn(
    const classic::SparseCsrMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});

classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::infer_scores(
    const classic::SparseTripletMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::infer_scores(
    const classic::SparseCsrMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::infer(
    const classic::SparseTripletMatrix& x, const classic::InferOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::infer(
    const classic::SparseCsrMatrix& x, const classic::InferOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::assess(
    const classic::SparseTripletMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::SparseLinearBinaryClassifier::assess(
    const classic::SparseCsrMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;

classic::StatusOr<std::vector<pluss::Series>> classic::SparseLinearBinaryClassifier::dump_state() const;
classic::Status classic::SparseLinearBinaryClassifier::load_state(
    /* a range of pluss::Series -- pass a std::vector<Series> or braced list */ state);
```

Every `learn`/`infer_scores`/`infer`/`assess` overload above exists twice
— once taking a [`SparseTripletMatrix`](SparseTripletMatrix.md) (which it
converts internally via
[`sparse_triplet_to_csr`](make_sparse_triplet.md)), once taking a
[`SparseCsrMatrix`](SparseCsrMatrix.md) directly (skipping that
conversion). Both forms of a given call produce identical results — see
[Example 2](#example-2).

## Construction

`SparseLinearBinaryClassifier()` takes no arguments and never fails —
there is no options struct fixed at construction the way
[`online::OnlineLinearClassifier`](../online/OnlineLinearClassifier.md)'s
constructor takes an
[`OnlineLinearOptions`](../online/OnlineLinearOptions.md). Every `learn`
call supplies its own `LinearFamilyOptions` instead, and only **three**
of that struct's fields are actually read here:

- `regularization` / `regularization_strength` — an L2 penalty (see
  [Example 5](#example-5)); any other `RegularizationKind` is silently
  treated as no penalty at all, never rejected.
- `objective` — **read but ignored.** This class always fits a `Logistic`
  model regardless of what `objective` is set to; no error, no different
  behavior — see [Example 3](#example-3).
- `fit_intercept` — **also ignored.** A bias term is always included; see
  [Example 4](#example-4).

`elastic_l1_ratio` and `robust_transition` are never read at all.

## State

`trained()` — `bool`
: `true` once `learn` has succeeded. Every other method except the
  constructor and `load_state` requires this.

`feature_count()` — `std::int64_t`
: The column count `learn` (or `load_state`) established — every later
  `infer`/`assess` call's matrix must have exactly this many columns.

The fitted bias-plus-weight vector itself is private, read back only
through `infer`/`infer_scores`/`assess`/`dump_state`.

## Methods

`learn(x, y, options = {}, linear_options = {})`
: Fits by full-batch gradient descent at a fixed learning rate of `0.05`
  (not configurable), running up to `options.max_iterations` steps and
  stopping early once successive losses differ by no more than
  `options.tolerance`. `y` need not hold exactly two values: `learn` maps
  every row to whichever of `y`'s own minimum or maximum value it is
  closer to (a tie goes to the maximum) — so a third, in-between label
  value is silently folded into whichever side it is nearer, not
  rejected. Only `y`'s minimum and maximum need differ at all — see
  [Example 6](#example-6). `options.max_iterations` must be `> 0` and
  `options.tolerance` must be `> 0` (both stricter than
  [`TrainOptions`](../core/TrainOptions.md)'s own defaults, which already
  satisfy this). Resets all fitted state — a second `learn` call discards
  the first fit entirely.

`infer_scores(x)`
: The raw linear score (`bias + weights · x`, no sigmoid) for each row —
  unbounded, not a probability.

`infer(x, options = {})`
: `sigmoid(score) >= options.positive_threshold`, mapped to `1.0`/`0.0`.
  **`options.positive_threshold` must be strictly inside `(0, 1)`** —
  exactly `0.0` or `1.0` is rejected outright, stricter than
  [`InferOptions`](../core/InferOptions.md)'s own documented range. See
  [Example 7](#example-7).

`assess(x, y, options = {})`
: Thresholds `y` itself at exactly `0.0` (`y > 0.0` counts as the
  positive class) — a **different**, simpler convention than `learn`'s
  own nearest-extreme mapping; the two can disagree on the same `y` — see
  [Example 7](#example-7). `options.metrics` empty (the default)
  requests `{Accuracy}`. Supports `Accuracy`, `BalancedAccuracy`,
  `Precision`, `Recall`/`Sensitivity`, `F1`, `Specificity`,
  `GeometricMean`, `BalanceQuality`, `LogLoss`, `Brier`, `Mcc`,
  `CohenKappa` — every other `MetricKind` is rejected with
  `Status::not_implemented`, not `Status::invalid_argument` — see
  [Example 10](#example-10).

`dump_state()` / `load_state(state)`
: `[metadata, weights]` — `metadata` is `[trained flag, feature_count]`;
  `weights` is the bias-plus-coefficient vector, length
  `feature_count() + 1`. See [Example 9](#example-9) for a round trip.

## Errors

- `learn`: `options.max_iterations <= 0` or `options.tolerance <= 0` —
  `Status::invalid_argument`.
- `learn`: `y` undefined, its length not matching the row count, non-finite,
  or its minimum equal to its maximum (only one effective class) —
  `Status::invalid_argument` / `Status::shape_mismatch`.
- `learn`: the fit diverges (non-finite loss or coefficients, typically
  from ill-conditioned data) — `Status::numerical_failure`.
- `infer_scores` / `infer` / `assess`: called before a successful `learn`
  — `Status::precondition`.
- `infer_scores` / `infer` / `assess`: the matrix's column count does not
  match `feature_count()` — `Status::shape_mismatch`.
- `infer`: `options.positive_threshold` not strictly inside `(0, 1)` —
  `Status::invalid_argument`.
- `assess`: `y` undefined, or its length not matching the row count —
  `Status::invalid_argument` / `Status::shape_mismatch`.
- `assess`: a requested `MetricKind` this class does not support —
  `Status::not_implemented`.
- `load_state`: fewer than 2 tensors, or a coefficient-vector length that
  does not match the stored `feature_count` — `Status::invalid_argument` /
  `Status::shape_mismatch`.

## Practical Examples

The full `learn`/`infer_scores`/`infer`/`assess` lifecycle, the
`SparseCsrMatrix` overload proven identical to the `SparseTripletMatrix`
one, `objective` and `fit_intercept` both proven to have zero effect, L2
regularization actually shrinking the fit, the nearest-extreme label
mapping on a three-valued `y`, `infer`'s `positive_threshold` range check
alongside `assess`'s different label convention, an error roundup, a
`dump_state`/`load_state` round trip, and the unsupported-metric
rejection.

### Example 1

Four rows, two features standing in for two vocabulary terms — rows
dominated by feature `0` are one class, rows dominated by feature `1` are
the other.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier model;
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "trained()      = " << model.trained() << "\n";
  std::cout << "feature_count() = " << model.feature_count() << "\n";

  std::cout << "infer_scores = " << model.infer_scores(x).value() << "\n";
  std::cout << "infer        = " << model.infer(x).value() << "\n";
  std::cout << "assess (Accuracy) = " << model.assess(x, y).value() << "\n";

  return 0;
}
```

Output

```text
learn ok = true
trained()      = true
feature_count() = 2
infer_scores = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.7513019, 5.5026037, -2.7513019, -5.5026037]
infer        = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 0, 0]
assess (Accuracy) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

The `SparseCsrMatrix` overload — the same data, pre-converted, fit and
scored identically to Example 1's `SparseTripletMatrix` form.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(x).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier via_triplet;
  via_triplet.learn(x, y);
  cls::SparseLinearBinaryClassifier via_csr;
  via_csr.learn(csr, y);

  const double diff = (via_triplet.infer_scores(x).value() - via_csr.infer_scores(csr).value())
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

`linear_options.objective` set to `LeastSquares` (a regression-only
value) has no effect at all — the fit is identical to the default
`Logistic`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier default_objective;
  default_objective.learn(x, y);

  cls::LinearFamilyOptions weird{};
  weird.objective = cls::LinearObjective::LeastSquares;
  cls::SparseLinearBinaryClassifier odd_objective;
  odd_objective.learn(x, y, {}, weird);

  const double diff = (default_objective.infer_scores(x).value() - odd_objective.infer_scores(x).value())
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

`linear_options.fit_intercept = false` also has no effect — a bias term
is always fit regardless.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier with_default;
  with_default.learn(x, y);

  cls::LinearFamilyOptions no_intercept{};
  no_intercept.fit_intercept = false;
  cls::SparseLinearBinaryClassifier without_intercept_flag;
  without_intercept_flag.learn(x, y, {}, no_intercept);

  const double diff = (with_default.infer_scores(x).value() - without_intercept_flag.infer_scores(x).value())
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

`regularization = L2` genuinely shrinks the fitted weights toward zero,
unlike `objective`/`fit_intercept` above.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier none;
  none.learn(x, y);

  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  cls::SparseLinearBinaryClassifier regularized;
  regularized.learn(x, y, {}, l2);

  std::cout << "score(row 0), no penalty = " << none.infer_scores(x).value() << "\n";
  std::cout << "score(row 0), L2 penalty = " << regularized.infer_scores(x).value() << "\n";

  return 0;
}
```

Output

```text
score(row 0), no penalty = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.7513019, 5.5026037, -2.7513019, -5.5026037]
score(row 0), L2 penalty = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.28724149, 0.57448298, -0.28724149, -0.57448298]
```

### Example 6

`y` holding three distinct values: `learn` maps each row to whichever of
`y`'s minimum (`0.0`) or maximum (`5.0`) it lands closer to — the
in-between value `1.0` (distance `1` to the minimum, `4` to the maximum)
folds into the "negative" (minimum) class rather than being rejected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      3, 1, {0, 1, 2}, {0, 0, 0}, {1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 1.0, 5.0}, {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier model;
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  // row 1 (y=1.0) was folded into the same class as row 0 (y=0.0) at fit
  // time -- both should score on the same side.
  std::cout << "infer = " << model.infer(x).value() << "\n";

  return 0;
}
```

Output

```text
learn ok = true
infer = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 1]
```

### Example 7

`infer`'s `positive_threshold` must be strictly inside `(0, 1)` — exactly
`1.0` is rejected. Separately, `assess`'s own label convention
(`y > 0.0`) can disagree with `learn`'s nearest-extreme mapping on the
same `y`: a small positive value below `learn`'s own midpoint is folded
into the "negative" class at fit time, but `assess` always counts it as
positive.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      3, 1, {0, 1, 2}, {0, 0, 0}, {1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 1.0, 5.0}, {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier model;
  model.learn(x, y);

  cls::InferOptions bad_threshold{};
  bad_threshold.positive_threshold = 1.0;
  const cls::StatusOr<pluss::Series> rejected = model.infer(x, bad_threshold);
  std::cout << "positive_threshold=1.0 ok = " << std::boolalpha << rejected.ok() << "\n";

  // assess's own convention: y=1.0 counts as positive (y > 0.0) even
  // though learn() folded it into the negative class above.
  cls::AssessOptions recall_only{};
  recall_only.metrics = {cls::MetricKind::Recall};
  std::cout << "assess Recall (y=1.0 treated as positive) = " << model.assess(x, y, recall_only).value() << "\n";

  return 0;
}
```

Output

```text
positive_threshold=1.0 ok = false
assess Recall (y=1.0 treated as positive) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.5]
```

### Example 8

Four distinct error conditions: `max_iterations <= 0`, `tolerance <= 0`,
a single-class `y` (minimum equals maximum), and an `infer` feature-count
mismatch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::TrainOptions zero_iters{};
  zero_iters.max_iterations = 0;
  cls::SparseLinearBinaryClassifier a;
  std::cout << "max_iterations=0 ok = " << std::boolalpha << a.learn(x, y, zero_iters).ok() << "\n";

  cls::TrainOptions zero_tol{};
  zero_tol.tolerance = 0.0;
  cls::SparseLinearBinaryClassifier b;
  std::cout << "tolerance=0 ok       = " << b.learn(x, y, zero_tol).ok() << "\n";

  const pluss::Series single_class = pluss::make_series(
      {1.0, 1.0, 1.0, 1.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier c;
  std::cout << "single-class y ok    = " << c.learn(x, single_class).ok() << "\n";

  cls::SparseLinearBinaryClassifier trained;
  trained.learn(x, y);
  const cls::SparseTripletMatrix wrong_cols = cls::make_sparse_triplet(1, 5, {0}, {4}, {1.0}).value();
  std::cout << "feature-count mismatch ok = " << trained.infer_scores(wrong_cols).ok() << "\n";

  return 0;
}
```

Output

```text
max_iterations=0 ok = false
tolerance=0 ok       = false
single-class y ok    = false
feature-count mismatch ok = false
```

### Example 9

`dump_state` / `load_state` round trip — a freshly-restored classifier
predicts identically to the original.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearBinaryClassifier original;
  original.learn(x, y);
  const std::vector<pluss::Series> state = original.dump_state().value();

  cls::SparseLinearBinaryClassifier restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.infer_scores(x).value() - restored.infer_scores(x).value())
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

### Example 10

An unsupported metric (`RocAuc`, which needs a full score sweep this
class does not keep) is rejected with `Status::not_implemented`, not
`Status::invalid_argument`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      4, 2, {0, 1, 2, 3}, {0, 0, 1, 1}, {1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 0.0, 0.0}, {4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier model;
  model.learn(x, y);

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::RocAuc};
  const cls::StatusOr<pluss::Series> rejected = model.assess(x, y, unsupported);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "status code = " << static_cast<int>(rejected.status().code) << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
status code = 4
message = "SparseLinearBinaryClassifier::assess supports hard-label / LogLoss / Brier / Mcc / CohenKappa metrics (Gini / AveragePrecision need a dense classifier's score sweep)."
```

## Changelog

- **0.1** — constructor, `trained`/`feature_count`, `learn`/`infer_scores`/
  `infer`/`assess` (`SparseTripletMatrix` and `SparseCsrMatrix` overloads),
  `dump_state`/`load_state`.

## See Also

- [`SparseLinearRegressor`](SparseLinearRegressor.md) /
  [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — the
  regression and multiclass counterparts.
- [`cross_assess_sparse`](cross_assess_sparse.md) — cross-validated
  scoring built on this class.
- [`SparseTripletMatrix`](SparseTripletMatrix.md) / [`SparseCsrMatrix`](SparseCsrMatrix.md) —
  the two input forms.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
