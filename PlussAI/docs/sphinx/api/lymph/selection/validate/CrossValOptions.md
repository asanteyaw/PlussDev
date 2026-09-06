# `lymph::options::CrossVal`

`options::CrossVal` (family alias `selection::CrossValOptions`, and
`selection::TimeValOptions` for the chronological reading) is the option struct
behind every [`Plan`](Plan.md). The [`cross`](cross.md) /
[`stratified`](cross.md#the-stratified-and-grouped-variants) /
[`grouped`](cross.md#the-stratified-and-grouped-variants) / [`time`](time.md)
factories each set `plan` and hand the rest to `Plan`, where the chained setters
refine it further. `Plan::options()` reads it back.

```cpp
struct CrossValOptions {
  CVPlanKind    plan                 = CVPlanKind::CrossVal;
  double        holdout_fraction     = 0.2;
  std::int64_t  folds                = 5;
  std::int64_t  repeats              = 1;
  std::int64_t  time_gap             = 0;
  std::int64_t  time_test_size       = 0;
  std::int64_t  time_max_train_size  = 0;
  bool          stratified           = false;
  Series        groups               = Series::undefined();
  bool          reliability_defaults = true;
  bool          shuffle              = true;
  std::int64_t  max_parallel         = 0;
  std::uint64_t random_state         = 0;
  bool          deterministic        = true;
};
```

## Fields

### `plan`

`CVPlanKind plan = CVPlanKind::CrossVal`

Which fold-assignment rule to use: `CrossVal` (plain k-fold),
`StratifiedCrossVal`, `GroupCrossVal`, or `TSCrossVal` (chronological). The
factory functions set this — you rarely write it by hand.

### `folds`

`std::int64_t folds = 5`

Number of folds. Must be greater than 1. Setter: `Plan::folds`.

### `repeats`

`std::int64_t repeats = 1`

How many times to repeat the whole k-fold split (each with a fresh shuffle),
pooling all `repeats * folds` folds. Setter: `Plan::repeat`.

### `stratified`

`bool stratified = false`

Set by [`stratified()`](cross.md#the-stratified-and-grouped-variants) alongside
`plan = StratifiedCrossVal`; keeps each fold's class balance close to the whole
set's.

### `groups`

`Series groups = Series::undefined()`

The per-row group key for
[`grouped()`](cross.md#the-stratified-and-grouped-variants). A group never
straddles the train/test boundary.

### `time_gap`

`std::int64_t time_gap = 0`

**`time` plans only.** Rows skipped between a fold's training window and its test
window — model a forecast lead time. Setter: `Plan::gap`.

### `time_test_size`

`std::int64_t time_test_size = 0`

**`time` plans only.** Test rows per chronological fold. Setter: `Plan::horizon`.

### `time_max_train_size`

`std::int64_t time_max_train_size = 0`

**`time` plans only.** Cap on training rows (`0` = expanding window, no cap; a
positive value makes it a sliding window). Setter: `Plan::lookback`. When run
through a time-aware [`workflow::Workflow`](../../workflow/Workflow.md), any unset
(`0`) `time_*` field is filled from the workflow's `TimeOptions`.

### `shuffle`

`bool shuffle = true`

Shuffle rows before splitting. Forced to `false` for `time` plans. Setter:
`Plan::shuffle`.

### `random_state`

`std::uint64_t random_state = 0`

The shuffle RNG seed. `Plan::seed(n)` sets this and `deterministic` together.

### `deterministic`

`bool deterministic = true`

Whether fold assignment is reproducible for a fixed `random_state`.

### `max_parallel`

`std::int64_t max_parallel = 0`

Maximum folds fitted concurrently; `0` lets the library choose. Setter:
`Plan::workers`.

### `holdout_fraction`

`double holdout_fraction = 0.2`

The test fraction used when a plan degrades to a single split (no valid k-fold
partition for the data). Rarely set directly.

### `reliability_defaults`

`bool reliability_defaults = true`

Apply the family's conservative defaults for reliability-style metrics inside
each fold's assessment.

## Related Pages

- [`Plan`](Plan.md) — the object these options configure.
- [`cross`](cross.md) / [`time`](time.md) — the factories that set `plan`.
- [`reports::CrossVal`](../../metrics/CrossValidationReport.md) — the report a
  `Plan` built from these options produces.
