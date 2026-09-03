<!-- pluss-hand-authored -->

# `lymph::selection::validate::cross`

[Back to `selection::validate`](../validate.md)

`cross` builds a plain **k-fold cross-validation** [`Plan`](Plan.md). Its two
close relatives, [`stratified`](#the-stratified-and-grouped-variants) and
[`grouped`](#the-stratified-and-grouped-variants), differ only in how rows are
assigned to folds — same return type, same `Plan` surface.

## Signature

```cpp
lymph::selection::Plan lymph::selection::cross(
    lymph::options::CrossVal options = {});

lymph::selection::Plan lymph::selection::stratified(
    lymph::options::CrossVal options = {});

lymph::selection::Plan lymph::selection::grouped(
    pluss::Series groups, lymph::options::CrossVal options = {});
```

`lymph::selection::validate::cross` is the full spelling;
`lymph::selection::cross` is the re-exported short form.

## What It Does

Each factory returns a [`Plan`](Plan.md) with `options.plan` set to the matching
`CVPlanKind` — `CrossVal`, `StratifiedCrossVal`, or `GroupCrossVal`. Nothing is
fitted yet; the plan is a contract. Chain `folds`, `repeat`, `shuffle`, `seed`,
`workers` to refine it, then `run` / `scores` / `score` it against a model.

- **`cross`** — rows shuffled (unless disabled) then dealt into `folds` equal
  parts.
- **`stratified`** — each fold's class balance is held close to the whole set's.
  Use it whenever the label is imbalanced.
- **`grouped`** — every row carries a group key (passed as `groups`), and a
  group is never split across the train/test boundary. Use it when rows share a
  leak-prone identity: the same customer, device, or subject.

## Parameters

`lymph::options::CrossVal options`
: The starting configuration — `folds`, `repeats`, `shuffle`, `random_state`,
  and so on (see [`options::CrossVal`](CrossValOptions.md)). Defaulted, so
  `cross()` is the common call; the plan's setters cover the rest.

`pluss::Series groups` *(grouped only)*
: One group id per row, same length as the frame. Rows with the same id always
  land on the same side of a fold.

## Returns

A `lymph::selection::Plan`. Cheap to copy; holds no fitted state.

## Errors

- The factories themselves do not throw.
- `Plan::folds(n)` with `n <= 1` throws; `grouped` with a `groups` length that
  does not match the data throws at `run` time.

## The stratified and grouped variants

`stratified()` and `grouped(groups)` are documented here because they are the
same operation as `cross()` with a different fold rule. Reach for `stratified`
on any skewed classification target; reach for `grouped` when repeated rows
share an identity that must not leak between train and test.

## Practical Examples

<!-- these examples establish: the run -> board shape, per-fold scores, repeated CV, and the stratified / grouped variants. -->

### Example 1

`cross().folds(4)` run against a logistic model. `run` returns a
[`reports::CrossVal`](../../metrics/CrossValidationReport.md) (the same type the
`metrics` family calls `CrossValidationReport`); its `board()` is the per-metric
summary.

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
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Thread(income)}, {"balance", Thread(balance)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).shuffle(true).seed(20260901ULL);

  const lymph::reports::CrossVal report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
folds = 4

===========================================
     | metric    mean          stdev       
-------------------------------------------
0    | accuracy  0.8541666667  0.1487797589
1    | f1        0.825         0.2046338193
-------------------------------------------
[2 cs x 3 feat]
```

### Example 2

`scores` returns the raw per-fold values for one metric; `score` is their mean.

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
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Thread(income)}, {"balance", Thread(balance)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan = lymph::selection::cross().folds(4).seed(20260901ULL);
  const lymph::models::Model model = lymph::models::logistic_regression();

  const pluss::Series per_fold =
      plan.scores(model, frame, data_options, lymph::MetricKind::Accuracy);
  const double mean_accuracy =
      plan.score(model, frame, data_options, lymph::MetricKind::Accuracy);

  std::cout << "per-fold accuracy:\n" << per_fold << "\n";
  std::cout << "mean accuracy = " << mean_accuracy << "\n";

  return 0;
}
```

Output

```text
per-fold accuracy:
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.66666667, 1, 0.75]
mean accuracy = 0.854167
```

### Example 3

`repeat(3)` runs the whole 4-fold split three times with different shuffles and
pools all 12 folds — a tighter estimate on small data.

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
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Thread(income)}, {"balance", Thread(balance)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).repeat(3).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options);

  std::cout << "folds = " << report.report.folds.size()
            << " (4 folds x 3 repeats)\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
folds = 12 (4 folds x 3 repeats)

===========================================
     | metric    mean          stdev       
-------------------------------------------
0    | accuracy  0.9097222222  0.1100206911
-------------------------------------------
[1 cs x 3 feat]
```

### Example 4

`stratified()` on a skewed label (15 negatives, 5 positives). Every fold gets
positives, so every fold is scorable.

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

  const pluss::Series score = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8,
       3.0, 3.2, 3.4, 3.6, 3.8, 6.0, 6.4, 6.8, 7.2, 7.6}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"score", Thread(score)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"score"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::stratified().folds(5).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "plan kind = " << static_cast<int>(plan.options().plan)
            << " (stratified), folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
plan kind = 2 (stratified), folds = 5

============================
     | metric    mean  stdev
----------------------------
0    | accuracy  1     0    
1    | f1        1     0    
----------------------------
[2 cs x 3 feat]
```

### Example 5

`grouped(groups)` over four 5-row groups. The group key travels as its own
`Series`, not as a frame column.

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
      {1.0, 1.2, 1.4, 5.0, 5.2, 1.1, 1.3, 1.5, 5.1, 5.3,
       1.0, 1.2, 1.4, 5.0, 5.2, 1.1, 1.3, 1.5, 5.1, 5.3}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1}, {20}, f64);
  const pluss::Series groups = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3}, {20}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::selection::Plan plan =
      lymph::selection::grouped(groups).folds(4).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options);

  std::cout << "plan kind = " << static_cast<int>(plan.options().plan)
            << " (grouped), folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
plan kind = 3 (grouped), folds = 4

============================
     | metric    mean  stdev
----------------------------
0    | accuracy  1     0    
----------------------------
[1 cs x 3 feat]
```

## See Also

- [`Plan`](Plan.md) — the object all three factories return.
- [`options::CrossVal`](CrossValOptions.md) — the option fields.
- [`time`](time.md) — the chronological plan factory.
- [`holdout`](holdout.md) — a single split instead of k folds.
