# `pluss::ai::classic::prep::NeighborImputer`

Fill a missing value from the *k* training rows most similar to it, judged
by every **other** feature both rows have present — a k-nearest-neighbors
average applied to filling a value instead of predicting a label. Reach for
this over [`SimpleImputer`](SimpleImputer.md) when rows with similar
feature profiles genuinely tend to share similar values for the missing one
too; it costs an `O(training rows)` scan per missing value at `map` time
(the whole training matrix is retained), unlike `SimpleImputer`'s O(1)
lookup.

## Signature

```cpp
explicit classic::prep::NeighborImputer(classic::prep::NeighborImputerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`NeighborImputerOptions`](NeighborImputerOptions.md).

## State

The entire training matrix (values, sample count) and the fitted per-feature
mean fallback live inside the object, set by `learn`, read back only
through `map` / `dump_state`. `trained()` reports whether `learn` has
succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Retains the entire training matrix verbatim (needed at `map` time to find
  neighbors) and computes each feature's overall mean over its non-`NaN`
  values as `empty_feature_fallback` otherwise — this fallback covers both
  a feature with no non-`NaN` training values, and (see `map`) a query row
  for which no training row shares any overlapping non-missing feature.
  Fails with `Status::invalid_argument` if `neighbors <= 0` or
  `distance_floor` is not a positive finite number.

`map(batch, options = {})`
: For each missing value in feature `f`, row `i`: finds every training row
  that has feature `f` present, computes squared Euclidean distance to row
  `i` using **only the other features both rows have present** (a training
  row sharing zero overlapping present features with row `i` is excluded
  entirely, not treated as distance `0`), keeps the `neighbors` closest, and
  averages their feature-`f` values — `Uniform` weighting (a plain mean) or
  `Distance` weighting (inverse-distance weighted; a training row at
  distance²  `<= distance_floor` is treated as an exact match: once any
  exact match exists among the kept neighbors, farther ones are excluded
  from the average entirely rather than contributing a tiny weight).
  Present values pass through unchanged. If no training row overlaps at
  all, the row falls back to that feature's fitted mean.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, train_values, fallback]` — the retained training matrix
  itself is part of the fitted state, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `neighbors <= 0` — `Status::invalid_argument` from `learn`.
- `distance_floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument`
  from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The default (`Distance`-weighted) fill on a small training set, `Uniform`
weighting on the same data for comparison, and the fallback-to-mean path
when a query row shares no overlapping feature with any training row.

### Example 1

Four training rows across two features; a missing value in the third row
filled from its nearest neighbors by the other feature's value.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // feature 0 is the "similarity" axis; feature 1 is what we fill.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 10.0, 1.0, 11.0, 5.0, 50.0, 6.0, 51.0}, {8}, f64).reshape({4, 2});

  cls::prep::NeighborImputerOptions options{};
  options.neighbors = 2;
  cls::prep::NeighborImputer imputer(options);
  imputer.learn(train);

  // query is closest (on feature 0) to the first two training rows.
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.5, nan}, {2}, f64).reshape({1, 2});
  std::cout << "map([0.5, NaN]) = " << imputer.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([0.5, NaN]) = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.5, 10.5]]
```

### Example 2

`Uniform` versus `Distance` weighting on the same query — `Distance` leans
harder toward the single closest neighbor.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.0, 1.0, 100.0, 10.0, 200.0}, {6}, f64).reshape({3, 2});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.5, nan}, {2}, f64).reshape({1, 2});

  cls::prep::NeighborImputerOptions uniform_opts{};
  uniform_opts.neighbors = 3;
  uniform_opts.weighting = cls::prep::NeighborImputerWeighting::Uniform;
  cls::prep::NeighborImputer uniform(uniform_opts);
  uniform.learn(train);
  std::cout << "Uniform:  " << uniform.map(probe).value() << "\n";

  cls::prep::NeighborImputerOptions distance_opts{};
  distance_opts.neighbors = 3;
  distance_opts.weighting = cls::prep::NeighborImputerWeighting::Distance;
  cls::prep::NeighborImputer distance(distance_opts);
  distance.learn(train);
  std::cout << "Distance: " << distance.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
Uniform:  pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.5, 100]]
Distance: pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.5, 53.846154]]
```

### Example 3

A query row that shares no non-missing feature with any training row falls
back to the fitted per-feature mean instead of failing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});  // mean = 20

  cls::prep::NeighborImputer imputer;
  imputer.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({nan}, {1}, f64).reshape({1, 1});  // no other feature to compare on
  std::cout << "no overlapping feature -> " << imputer.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
no overlapping feature -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[20]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `neighbors`/`weighting`/
  `distance_floor`/`empty_feature_fallback`, `dump_state`/`load_state`.

## See Also

- [`NeighborImputerOptions`](NeighborImputerOptions.md) — the options
  struct.
- [`SimpleImputer`](SimpleImputer.md) — the cheaper, feature-independent
  alternative.
- [`imputation/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

NeighborImputerOptions
```
<!-- apiref:toctree:end -->
