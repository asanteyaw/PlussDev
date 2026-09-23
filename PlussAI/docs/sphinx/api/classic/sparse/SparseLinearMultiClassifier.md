# `pluss::ai::classic::SparseLinearMultiClassifier`

A one-vs-rest multiclass classifier fit directly on a sparse matrix —
fits one [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md)-style
logistic model per distinct class in `y`, predicts by the highest raw
score across them. Shares the same no-constructor-options,
per-`learn`-call `LinearFamilyOptions` shape as
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
[`SparseLinearRegressor`](SparseLinearRegressor.md).

## Signature

```cpp
classic::SparseLinearMultiClassifier();

bool classic::SparseLinearMultiClassifier::trained() const noexcept;
std::int64_t classic::SparseLinearMultiClassifier::feature_count() const noexcept;
std::int64_t classic::SparseLinearMultiClassifier::class_count() const noexcept;

classic::Status classic::SparseLinearMultiClassifier::learn(
    const classic::SparseTripletMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});
classic::Status classic::SparseLinearMultiClassifier::learn(
    const classic::SparseCsrMatrix& x,
    const pluss::Series& y,
    const classic::TrainOptions& options = {},
    const classic::LinearFamilyOptions& linear_options = {});

classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::infer_scores(
    const classic::SparseTripletMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::infer_scores(
    const classic::SparseCsrMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::infer(
    const classic::SparseTripletMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::infer(
    const classic::SparseCsrMatrix& x) const;
classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::assess(
    const classic::SparseTripletMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::SparseLinearMultiClassifier::assess(
    const classic::SparseCsrMatrix& x, const pluss::Series& y, const classic::AssessOptions& options = {}) const;

classic::StatusOr<std::vector<pluss::Series>> classic::SparseLinearMultiClassifier::dump_state() const;
classic::Status classic::SparseLinearMultiClassifier::load_state(
    /* a range of pluss::Series -- pass a std::vector<Series> or braced list */ state);
```

## Construction

`SparseLinearMultiClassifier()` takes no arguments; `LinearFamilyOptions`
is supplied fresh per `learn` call, with the same three-fields-matter
contract as
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md#construction) —
`regularization`/`regularization_strength` apply to every per-class fit;
`objective` (always `Logistic` per class, regardless of what is set) and
`fit_intercept` (always included) are both ignored.

## State

`trained()` — `bool` — `true` once `learn` has succeeded.

`feature_count()` — `std::int64_t` — the column count established at
`learn`/`load_state`.

`class_count()` — `std::int64_t` — how many distinct values `learn` found
in `y` — **unlike**
[`SparseLinearBinaryClassifier::learn`](SparseLinearBinaryClassifier.md#methods),
which tolerates any number of distinct `y` values by folding them toward
two extremes, this class fits one model per distinct value found, however
many there are (at least two — see `## Errors`).

## Methods

`learn(x, y, options = {}, linear_options = {})`
: Finds every distinct value in `y` (sorted), then fits one
  [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md)-style
  one-vs-rest model per class, each against the same `x`. Requires at
  least two distinct values in `y`. `options.max_iterations`/
  `options.tolerance` govern every one of those per-class fits, with the
  same `> 0` requirement as
  [`SparseLinearBinaryClassifier::learn`](SparseLinearBinaryClassifier.md#methods).
  Resets all fitted state on every call.

`infer_scores(x)`
: `[rows, class_count()]` — each column's raw one-vs-rest score for that
  class (no softmax, no normalization across the row).

`infer(x)`
: The class value (not an index) whose column had the highest score per
  row — a tie keeps the **first** (lowest sorted) class value among the
  tied scores.

`assess(x, y, options = {})`
: `y` must contain only values `learn` actually saw — a genuinely new
  class fails outright (`Status::invalid_argument`), unlike
  [`SparseLinearBinaryClassifier::assess`](SparseLinearBinaryClassifier.md#methods)'s
  looser `y > 0.0` convention. `options.metrics` empty (the default)
  requests `{Accuracy}`. Supports `Accuracy`, `BalancedAccuracy`,
  `Precision`, `Recall`/`Sensitivity`, `F1`, `Specificity`,
  `GeometricMean`, `BalanceQuality`, `Mcc`, `CohenKappa` — every value is
  the **macro** (unweighted per-class) average; there is no micro/weighted
  option the way
  [`ClassReportOptions`](../metrics/ClassReportOptions.md) offers.
  `BalancedAccuracy`, `Recall`, and `Sensitivity` are therefore always
  identical here (all three read the same macro-recall value) — see
  [Example 5](#example-5). `LogLoss`/`Brier` are not supported at all
  (only the binary classifier computes them); requesting them, or
  `RocAuc`/`Gini`/`AveragePrecision`, fails with `Status::not_implemented`.

`dump_state()` / `load_state(state)`
: `[metadata, class_values, weights]` — **three** tensors, one more than
  [`SparseLinearBinaryClassifier`'s two](SparseLinearBinaryClassifier.md#methods) —
  `class_values` is the sorted class list; `weights` is every class's
  bias-plus-coefficient block, flattened back to back in that same order.

## Errors

- `learn`: `options.max_iterations <= 0` or `options.tolerance <= 0` —
  `Status::invalid_argument`.
- `learn`: `y` undefined, its length not matching the row count,
  non-finite, or fewer than two distinct values — `Status::invalid_argument` /
  `Status::shape_mismatch`.
- `learn`: any per-class fit diverges — `Status::numerical_failure`.
- `infer_scores` / `infer` / `assess`: called before a successful `learn`
  — `Status::precondition`.
- `infer_scores` / `infer` / `assess`: the matrix's column count does not
  match `feature_count()` — `Status::shape_mismatch`.
- `assess`: `y` undefined, its length not matching the row count, or
  containing a value `learn` never saw — `Status::invalid_argument` /
  `Status::shape_mismatch`.
- `assess`: a requested `MetricKind` this class does not support —
  `Status::not_implemented`.
- `load_state`: fewer than 3 tensors, fewer than two class values, or a
  weight-vector length mismatch — `Status::invalid_argument` /
  `Status::shape_mismatch`.

## Practical Examples

The full lifecycle across three classes, the `SparseCsrMatrix` overload
proven identical, `assess`'s strict unseen-class rejection, the
`BalancedAccuracy`/`Recall`/`Sensitivity` equivalence, a `dump_state`/
`load_state` round trip, and an unsupported-metric rejection.

### Example 1

Three classes, one feature each mostly identifies.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearMultiClassifier model;
  const cls::Status learned = model.learn(x, y);
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "class_count() = " << model.class_count() << "\n";
  std::cout << "infer = " << model.infer(x).value() << "\n";
  std::cout << "assess (default Accuracy) = " << model.assess(x, y).value() << "\n";

  return 0;
}
```

Output

```text
learn ok = true
class_count() = 3
infer = pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 1, 1, 2, 2]
assess (default Accuracy) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

The `SparseCsrMatrix` overload, proven identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(x).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearMultiClassifier via_triplet;
  via_triplet.learn(x, y);
  cls::SparseLinearMultiClassifier via_csr;
  via_csr.learn(csr, y);

  const double diff = (via_triplet.infer_scores(x).value() - via_csr.infer_scores(csr).value())
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between the two forms = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between the two forms = 0
```

### Example 3

`assess` rejects a truth value `learn` never saw as a class.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearMultiClassifier model;
  model.learn(x, y);

  const pluss::Series unseen_class = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 3.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  const cls::StatusOr<pluss::Series> rejected = model.assess(x, unseen_class);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "SparseLinearMultiClassifier::assess labels must match classes observed at learn()."
```

### Example 4

`dump_state` / `load_state` round trip — three tensors, including the
class value list.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearMultiClassifier original;
  original.learn(x, y);
  const std::vector<pluss::Series> state = original.dump_state().value();
  std::cout << "dump_state() tensor count = " << state.size() << "\n";

  cls::SparseLinearMultiClassifier restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.infer_scores(x).value() - restored.infer_scores(x).value())
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff after restore = " << diff << "\n";

  return 0;
}
```

Output

```text
dump_state() tensor count = 3
load_state ok = true
max abs diff after restore = 0
```

### Example 5

`BalancedAccuracy`, `Recall`, and `Sensitivity` are always identical for
this class — all three read the same macro-recall value.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearMultiClassifier model;
  model.learn(x, y);

  cls::AssessOptions three{};
  three.metrics = {cls::MetricKind::BalancedAccuracy, cls::MetricKind::Recall, cls::MetricKind::Sensitivity};
  std::cout << "[BalancedAccuracy, Recall, Sensitivity] = " << model.assess(x, y, three).value() << "\n";

  return 0;
}
```

Output

```text
[BalancedAccuracy, Recall, Sensitivity] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

### Example 6

`LogLoss` (only the binary classifier computes it) is rejected with
`Status::not_implemented`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearMultiClassifier model;
  model.learn(x, y);

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::LogLoss};
  const cls::StatusOr<pluss::Series> rejected = model.assess(x, y, unsupported);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "SparseLinearMultiClassifier::assess supports hard-label / Mcc / CohenKappa multiclass classification metrics (Gini / AveragePrecision / Brier need a dense classifier)."
```

## Changelog

- **0.1** — constructor, `trained`/`feature_count`/`class_count`,
  `learn`/`infer_scores`/`infer`/`assess` (`SparseTripletMatrix` and
  `SparseCsrMatrix` overloads), `dump_state`/`load_state`.

## See Also

- [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) — the
  two-class counterpart this fits one-vs-rest.
- [`SparseLinearRegressor`](SparseLinearRegressor.md) — the regression
  counterpart.
- [`cross_assess_sparse`](cross_assess_sparse.md) — cross-validated
  scoring built on this class.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
