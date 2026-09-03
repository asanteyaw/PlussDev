# `lymph::selection::validate::HoldoutPlan`

`HoldoutPlan` is a **single train/test split** contract — one cut, not k folds.
Built by [`holdout`](holdout.md); `split(frame)` produces a `prep::HoldoutSplit`
with `train_set` and `test_set`.

Reach for it when a full cross-validation is more than the question needs: a
quick sanity check, a fixed held-out set you re-use across experiments, or the
final "does it hold on data it never saw" pass after model selection.

## Scope And Alias

The type is `lymph::selection::validate::HoldoutPlan`, re-exported as
`lymph::selection::HoldoutPlan`.

## Construction

```cpp
explicit HoldoutPlan(selection::HoldoutOptions options = {});
```

Built by [`holdout(options = {})`](holdout.md), not by hand. The
[`options::HoldoutSplit`](HoldoutOptions.md) fields (`test_size`, `stratify`,
`shuffle`, `random_state`, `deterministic`) and their defaults are on that page.

## Configuration

Each setter returns `*this`; [`options()`](HoldoutOptions.md) reads the result
back.

`test_size(double)`
: Fraction of rows in the test set. Must be strictly between 0 and 1.

`stratify(std::string feature_name)` / `stratify(Series)`
: Keep the split's class balance close to the whole set's, keyed on a column
  name or an explicit `Series`. The two overloads are mutually exclusive —
  setting one clears the other.

`shuffle(bool = true)`
: Shuffle before cutting. With `shuffle(false)` the test set is the last
  `test_size` fraction of rows in their current order.

`seed(std::uint64_t)`
: Fix the shuffle RNG and set `deterministic = true`.

## Methods

### `split` → `prep::HoldoutSplit`

```cpp
prep::HoldoutSplit split(const pluss::table::DataFrame& frame) const;
prep::HoldoutSplit split(const LabeledBatch& batch) const;
```

Cut the input once. The `DataFrame` overload fills `train_set` / `test_set`
(and `has_frame()` is true); the `LabeledBatch` overload fills
`train_batch` / `test_batch` (and `has_batch()` is true). Row count is
preserved.

#### Example — split sizes and reproducibility

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  const lymph::selection::HoldoutPlan plan =
      lymph::selection::holdout().test_size(0.25).shuffle(true).seed(20260901ULL);

  const lymph::prep::HoldoutSplit split = plan.split(frame);
  std::cout << "train_rows = " << split.train_set.ncases()
            << ", test_rows = " << split.test_set.ncases()
            << ", total = " << (split.train_set.ncases() + split.test_set.ncases()) << "\n";
  std::cout << "test_size option = " << plan.options().test_size << "\n";

  return 0;
}
```

Output

```text
train_rows = 12, test_rows = 4, total = 16
test_size option = 0.25
```

#### Example — train on the split

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Thread(income)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income"};
  data_options.label_feature = "target";

  const lymph::prep::HoldoutSplit split =
      lymph::selection::holdout().test_size(0.3).stratify("target").seed(20260901ULL).split(frame);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(split.train_set, data_options);

  const lymph::metrics::MetricReport report = model.assess(
      split.test_set, data_options, {.metrics = {lymph::MetricKind::Accuracy}});

  std::cout << "train_rows = " << split.train_set.ncases()
            << ", test_rows = " << split.test_set.ncases() << "\n";
  std::cout << "held-out accuracy = " << report.accuracy.item<double>() << "\n";

  return 0;
}
```

Output

```text
train_rows = 14, test_rows = 6
held-out accuracy = 0.833333
```

## Notes

- `HoldoutPlan` and `Plan` are different types with different results:
  `HoldoutPlan::split` hands you frames to use yourself; `Plan::run` fits and
  scores for you. `holdout` builds the former, `cross` / `stratified` / … the
  latter.
- `stratify` on a regression target is not meaningful — leave it unset there.

## Changelog

- **0.1** — `HoldoutPlan` with `test_size` / `stratify` / `shuffle` / `seed` and
  `split` (frame / batch).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

HoldoutOptions
```
<!-- apiref:toctree:end -->
