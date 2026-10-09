<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 775732299d55 -->

# `lymph::selection::validate::Plan::splits`

[Back to `Plan`](../Plan.md)

`splits` and `last` expose the fold boundaries of a **`time` plan** directly, as
`prep::TimeSeriesSplit` frames — without fitting anything. `splits` returns every
expanding-window fold; `last` returns only the final one, the single
chronological holdout.

## Signature

```cpp
std::vector<prep::TimeSeriesSplit> splits(const pluss::table::DataFrame& frame,
                                          std::string_view time_feature) const;

prep::TimeSeriesSplit last(const pluss::table::DataFrame& frame,
                           std::string_view time_feature) const;
```

Both are **`time` plans only** — they raise on a `cross` / `stratified` /
`grouped` plan. `time_feature` must name a **Timeline / TimeIndex** column;
rows are sorted by it before the windows are cut.

## What It Does

A `time` plan splits the data into `folds` expanding windows: fold *i* trains on
the first *i* blocks and tests on block *i+1*. `splits` materializes all of them;
each `prep::TimeSeriesSplit` carries `.train_set` and `.test_set` frames (sorted
oldest-first) plus `.ncases()` on each. `last` is `splits(...).back()` — the
final train-on-all-history-so-far, test-on-the-tail split you would ship as the
real evaluation.

The window shape comes from the plan's setters:

- `horizon(n)` (`time_test_size`) — test rows per fold.
- `gap(n)` (`time_gap`) — rows skipped between a fold's train and test windows.
- `lookback(n)` (`time_max_train_size`) — cap on training rows; a positive value
  turns the expanding window into a sliding one.

`run` through `(frame, data_options)` folds on row order and needs the frame
already sorted; `splits` / `last` are the explicit, timeline-column form.

## Parameters

`frame` — `pluss::table::DataFrame`
: The data to cut. Any row order — it is sorted by `time_feature` first.

`time_feature` — `std::string_view`
: Name of the Timeline / TimeIndex column defining chronological order.

## Returns

`splits` → `std::vector<prep::TimeSeriesSplit>`, one per fold, chronological.

`last` → a single `prep::TimeSeriesSplit`, the final fold.

## Errors

- Raises if the plan is not a `time` plan.
- Raises if `time_feature` is missing or not a Timeline / TimeIndex column.

## Practical Examples

<!-- these examples establish: the expanding-window shape, the last() holdout, and the sliding window via lookback. -->

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
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Slate when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Slate(demand)}, {"target", Slate(y)},
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
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Slate when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Slate(demand)}, {"target", Slate(y)},
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

`lookback(6)` caps the training window. With it set, each fold drops its oldest
rows instead of accumulating them — the expanding window becomes a sliding one.
`gap(1)` skips one row between train and test.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 11., 12., 13., 14., 15., 16., 17., 18.,
       19., 20., 21., 22., 23., 24., 25., 26., 27.}, {18}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {18}, f64);
  const Slate when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z",
      "2026-01-13T00:00:00Z", "2026-01-14T00:00:00Z", "2026-01-15T00:00:00Z",
      "2026-01-16T00:00:00Z", "2026-01-17T00:00:00Z", "2026-01-18T00:00:00Z"}));
  const DataFrame frame({{"demand", Slate(demand)}, {"target", Slate(y)},
                         {"when", when}});

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;
  const lymph::selection::Plan sliding =
      lymph::selection::time(cv).lookback(6).gap(1);

  const auto splits = sliding.splits(frame, "when");
  std::cout << "sliding folds = " << splits.size() << "\n";
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
sliding folds = 3
  fold 0: train_rows=6 test_rows=3
  fold 1: train_rows=6 test_rows=3
  fold 2: train_rows=6 test_rows=3
```

## See Also

- [`time`](../time.md) — the plan factory these methods require.
- [`run`](run.md) — cross-validate a model over the same windows.
- [`holdout`](../holdout.md) — a single non-chronological split.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scores.html">
    <span class="label">Previous</span>
    <span class="title">scores</span>
  </a>
</div>
<!-- apiref:nav:end -->
