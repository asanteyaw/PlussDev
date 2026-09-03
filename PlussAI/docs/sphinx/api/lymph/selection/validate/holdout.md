<!-- pluss-hand-authored -->

# `lymph::selection::validate::holdout`

[Back to `selection::validate`](../validate.md)

`holdout` builds a [`HoldoutPlan`](HoldoutPlan.md) — a **single train/test
split**, not k folds. `split(frame)` returns the two frames; you train and judge
with them yourself.

## Signature

```cpp
lymph::selection::HoldoutPlan lymph::selection::holdout(
    lymph::options::HoldoutSplit options = {});
```

`lymph::selection::validate::holdout` is the full spelling;
`lymph::selection::holdout` the re-exported short form.

## What It Does

Returns a [`HoldoutPlan`](HoldoutPlan.md). Chain `test_size`, `stratify`,
`shuffle`, `seed` to configure it, then `split(frame)` (or `split(batch)`) to
cut the data once. The result is a `prep::HoldoutSplit` — `train_set` /
`test_set` frames (or `train_batch` / `test_batch`) with the row count
preserved.

Use it when a full cross-validation is more than the question needs: a quick
check, a fixed held-out set reused across experiments, or the final pass on data
the model-selection process never touched.

## Parameters

`lymph::options::HoldoutSplit options`
: Starting configuration — `test_size`, `stratify` / `stratify_feature`,
  `shuffle`, `random_state` (see [`options::HoldoutSplit`](HoldoutOptions.md)).
  Defaulted, so `holdout()` is the common call.

## Returns

A `lymph::selection::HoldoutPlan`. Cheap to copy; holds no fitted state.

## Errors

- `HoldoutPlan::test_size(v)` throws unless `0 < v < 1`.
- The two `stratify` overloads are mutually exclusive — setting one clears the
  other; that is by design, not an error.

## Practical Examples

<!-- these examples establish: split sizes, seed reproducibility, and training on the split. -->

### Example 1

`split(frame)` preserves the row count and honours `test_size`.

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

### Example 2

The same seed gives the same split — reproducible experiments.

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
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  const lymph::prep::HoldoutSplit a =
      lymph::selection::holdout().test_size(0.25).stratify("target").seed(7).split(frame);
  const lymph::prep::HoldoutSplit b =
      lymph::selection::holdout().test_size(0.25).stratify("target").seed(7).split(frame);

  std::cout << "run A: train=" << a.train_set.ncases() << " test=" << a.test_set.ncases() << "\n";
  std::cout << "run B: train=" << b.train_set.ncases() << " test=" << b.test_set.ncases() << "\n";
  std::cout << "same seed -> identical test labels: "
            << (a.test_set.features("target").as_series().to_vec<double>()
                == b.test_set.features("target").as_series().to_vec<double>()
                ? "yes" : "no")
            << "\n";

  return 0;
}
```

Output

```text
run A: train=12 test=4
run B: train=12 test=4
same seed -> identical test labels: yes
```

### Example 3

The practical loop: split once, `learn_` on `train_set`, `assess` on the
untouched `test_set`.

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

## See Also

- [`HoldoutPlan`](HoldoutPlan.md) — the object this returns.
- [`options::HoldoutSplit`](HoldoutOptions.md) — the option fields.
- [`cross`](cross.md) — k-fold cross-validation instead of one split.
