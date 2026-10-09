# `lymph::selection::validate::Plan`

`Plan` is the reusable **cross-validation contract** of the
[`selection::validate`](../validate.md) subfamily. It holds a validation policy —
fold count, shuffling, seeding, worker count, and (for chronological plans) the
time-window shape — as a value you configure once and run against as many models
and frames as you like.

You do not usually construct it directly. A factory sets the policy kind:
[`cross`](cross.md) (plain k-fold), [`stratified`](cross.md#the-stratified-and-grouped-variants)
(class-balanced folds), [`grouped`](cross.md#the-stratified-and-grouped-variants)
(folds respect a group key), or [`time`](time.md) (chronological, expanding
window). The chained setters then refine it.

## Scope And Alias

The type is `lymph::selection::validate::Plan`, re-exported one scope up as
`lymph::selection::Plan`. Both names are the same type; the shorter one is the
usual spelling.

Build a cross-validation plan and a stratified plan, then read the fold count from each plan's options. Both default to five folds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  lymph::selection::Plan a = lymph::selection::cross().folds(5);
  lymph::selection::validate::Plan b = lymph::selection::validate::stratified();
  std::cout << "cross folds = " << a.options().folds << "\n";
  std::cout << "stratified folds = " << b.options().folds << "\n";
  return 0;
}
```

Output

```text
cross folds = 5
stratified folds = 5
```

## Construction

```cpp
explicit Plan(selection::CrossValOptions options = {});
```

Built by a factory, not by hand: `cross`, `stratified`, `grouped`, `time`,
`time_ordered`. Each sets `options.plan` to the matching `CVPlanKind`; `grouped`
also takes the group-key `Series`. The
[`options::CrossVal`](CrossValOptions.md) fields and their defaults are on that
page.

## Configuration

Each setter returns `*this` and mutates the held options, so calls chain.
[`options()`](CrossValOptions.md) reads the result back.

`folds(std::int64_t)`
: Number of folds. Must be greater than 1.

`repeat(std::int64_t)`
: Run the whole k-fold split this many times with different shuffles and pool
  all `repeats * folds` folds. Tightens the estimate on small data.

`shuffle(bool = true)`
: Shuffle rows before splitting. Forced off for `time` plans.

`seed(std::uint64_t)`
: Fix the shuffle RNG and set `deterministic = true` — reproducible folds.

`workers(std::int64_t)`
: Max folds fitted in parallel (`0` = library default).

`gap(std::int64_t)` / `horizon(std::int64_t)` / `lookback(std::int64_t)`
: **`time` plans only.** Rows skipped between train and test, test rows per
  fold, and the cap on training rows (`0` = expand without limit; a positive
  value makes it a *sliding* window). See [`time`](time.md).

## Methods

| Method | Summary |
|--------|---------|
| [`run`](Plan/run.md) | Fit and score a model / spec / workflow across the folds → `reports::CrossVal`; a `Builder` overload ranks candidates → `selection::CompareResult`. |
| [`scores`](Plan/scores.md) | One metric's value per fold as a `Series` (`score` → their mean). |
| [`splits`](Plan/splits.md) | The fold boundaries of a `time` plan as `prep::TimeSeriesSplit` frames, without fitting (`last` → the final one). |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Plan/Introduction
Plan/run
Plan/scores
Plan/splits
CrossValOptions
```
<!-- apiref:toctree:end -->

## Changelog

- **0.1** — `Plan` with `folds` / `repeat` / `shuffle` / `seed` / `workers` /
  `gap` / `horizon` / `lookback`, `run` (model / spec / workflow / builder),
  `scores` / `score`, and `splits` / `last`.
