<!-- pluss-hand-authored -->

# `lymph::selection::validate::time`

[Back to `selection::validate`](../validate.md)

`time` builds a **chronological, expanding-window** cross-validation
[`Plan`](Plan.md): every test fold sits strictly after the rows it was trained
on, and shuffling is forced off. `time_ordered` is the same function — `time` is
the shorter alias.

## Signature

```cpp
lymph::selection::Plan lymph::selection::time(
    lymph::options::CrossVal options = {});

lymph::selection::Plan lymph::selection::time_ordered(
    lymph::options::CrossVal options = {});   // identical; `time` calls this
```

`lymph::selection::validate::time` is the full spelling; `lymph::selection::time`
the re-exported short form.

## What It Does

Returns a [`Plan`](Plan.md) with `options.plan = CVPlanKind::TSCrossVal` and
`shuffle` forced to `false`. The plan splits the data into `folds` expanding
windows: fold *i* trains on the first *i* blocks and tests on block *i+1*. Three
setters shape the windows:

- **`Plan::horizon(n)`** (`options.time_test_size`) — test rows per fold.
- **`Plan::gap(n)`** (`options.time_gap`) — rows skipped between a fold's train
  and test windows, to model a forecast lead time.
- **`Plan::lookback(n)`** (`options.time_max_train_size`) — cap on training
  rows (`0` = expand without limit; a positive value makes it a *sliding*
  window).

`run` cross-validates a model across these folds and returns a
[`reports::CrossVal`](../../metrics/CrossValidationReport.md) (the `metrics` family calls it `CrossValidationReport`), just
like a k-fold plan. `Plan::splits(frame, time_feature)` and
`Plan::last(frame, time_feature)` expose the fold boundaries directly — `splits`
every fold, `last` only the final one (the single chronological holdout). Those
two require a **Timeline / TimeIndex** column named by `time_feature`; `run`
through `(frame, data_options)` folds on row order and needs the frame already
sorted oldest-first.

## Parameters

`lymph::options::CrossVal options`
: Starting configuration. `folds`, `time_test_size`, `time_gap`,
  `time_max_train_size` are the relevant fields (see
  [`options::CrossVal`](CrossValOptions.md)); the rest come from the plan's
  setters. When the plan is run through a time-aware
  [`workflow::Workflow`](../../model/Workflow.md), any unset (`0`) window field
  is filled from the workflow's own `TimeOptions`.

## Returns

A `lymph::selection::Plan` of kind `TSCrossVal`.

## Errors

- `Plan::gap` / `horizon` / `lookback` reject negative values.
- `Plan::splits` / `Plan::last` raise if the plan is not a `time` plan, or if
  `time_feature` is not a Timeline/TimeIndex column.
- A `time` plan whose chronological test block ends up single-class fails at
  assess time (a classification metric needs both classes) — a real property of
  drifting labels, not a bug.

## Practical Examples

<!-- these examples establish: the expanding-window fold shape, the last() holdout, and run() as a full CV. -->

### Example 1

`splits(frame, "when")` hands back the ordered fold boundaries. Each fold trains
on more history than the last.

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

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Thread when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Thread(demand)}, {"target", Thread(y)},
                         {"when", when}});

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 2;
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const auto splits = plan.splits(frame, "when");
  std::cout << "chronological folds = " << splits.size() << "\n";
  for (std::size_t i = 0; i < splits.size(); ++i) {
    std::cout << "  fold " << i
              << ": train_rows=" << splits[i].train_set.ncases()
              << " test_rows=" << splits[i].test_set.ncases() << "\n";
  }

  return 0;
}
```

Output

```text
chronological folds = 3
  fold 0: train_rows=6 test_rows=2
  fold 1: train_rows=8 test_rows=2
  fold 2: train_rows=10 test_rows=2
```

### Example 2

`last(frame, "when")` is the single chronological holdout — train on all history
up to a cut, test on the final block.

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

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Thread when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Thread(demand)}, {"target", Thread(y)},
                         {"when", when}});

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;  // final 3 rows are the test block
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const lymph::prep::TimeSeriesSplit tail = plan.last(frame, "when");
  std::cout << "train_rows = " << tail.train_set.ncases()
            << ", test_rows = " << tail.test_set.ncases() << "\n";
  std::cout << "last train 'when' = " << tail.train_set.features("when").tail(1)
            << "first test 'when' = " << tail.test_set.features("when").head(1);

  return 0;
}
```

Output

```text
train_rows = 9, test_rows = 3
last train 'when' = 8  2026-01-09
Name: when, ptype: Timeline
first test 'when' = 9  2026-01-10
Name: when, ptype: Timeline
```

### Example 3

`run` on a `time` plan is a full cross-validation over the expanding windows —
here with a regressor and R² / MSE.

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

  const pluss::Series step = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20.}, {20}, f64);
  const pluss::Series demand = pluss::make_series(
      {12., 13.5, 15., 16.6, 18., 19.4, 21., 22.5, 24., 25.4,
       27., 28.6, 30., 31.5, 33., 34.4, 36., 37.5, 39., 40.6}, {20}, f64);
  const DataFrame frame({{"step", Thread(step)}, {"demand", Thread(demand)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"step"};
  data_options.label_feature = "demand";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::R2, lymph::MetricKind::Mse};

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::linear_regression(), frame, data_options, {}, assess);

  std::cout << "chronological folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
```

Output

```text
chronological folds = 3

=============================================
     | metric  mean            stdev         
---------------------------------------------
0    | r2      0.9961216221    0.001563293294
1    | mse     0.005741844689  0.002064729948
---------------------------------------------
[2 cs x 3 feat]
```

## See Also

- [`Plan`](Plan.md) — the object this returns; its `gap` / `horizon` /
  `lookback` setters shape the windows.
- [`options::CrossVal`](CrossValOptions.md) — the `time_*` fields.
- [`cross`](cross.md) — plain k-fold, when order does not matter.
- [`workflow::Workflow`](../../model/Workflow.md) — time-aware workflows fill the
  window fields automatically.
