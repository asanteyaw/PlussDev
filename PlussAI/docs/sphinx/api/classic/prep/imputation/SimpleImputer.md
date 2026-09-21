# `pluss::ai::classic::prep::SimpleImputer`

Fill every missing value in a feature with one fixed, per-feature statistic
— the mean, the median, the most-frequent value, or a constant you choose —
computed once from the training batch's non-`NaN` values. No relationship
between features is assumed or used, which makes this the fastest imputer
in the sub-family and the right default unless you have a specific reason a
row's other feature values should influence how its missing value gets
filled (see [`NeighborImputer`](NeighborImputer.md) /
[`IterativeImputer`](IterativeImputer.md) for that).

## Signature

```cpp
explicit classic::prep::SimpleImputer(classic::prep::SimpleImputerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`SimpleImputerOptions`](SimpleImputerOptions.md).

## State

The fitted per-feature fill value lives inside the object, set by `learn`,
read back only through `map` / `dump_state`. `trained()` reports whether
`learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature, computes the fill value per `strategy` from the
  column's non-`NaN` values: `Mean` (arithmetic mean), `Median` (linear-
  interpolated, as [`RobustScaler`](../scaling/RobustScaler.md) uses),
  `MostFrequent` (ties broken toward the smaller value), `Constant`
  (`constant_value`, ignoring the data entirely). A feature with zero
  non-`NaN` values fits `empty_feature_fallback` regardless of `strategy` —
  including `Constant`, since there is nothing to be constant *about* for a
  wholly-missing column either way in practice, though `Constant`'s fitted
  value equals `constant_value` whenever at least the column is present,
  fallback or not.

`map(batch, options = {})`
: Replaces every `NaN` with its feature's fitted fill value; every present
  value passes through unchanged. `Status::precondition` if not yet
  trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, fill_values]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

Each of the four strategies on the same feature, and the
`empty_feature_fallback` an entirely-missing feature fits regardless of
strategy.

### Example 1

`Mean` (the default) versus `Median` on a column with an outlier that
pulls the mean away from the bulk of the data.

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
  train.x = pluss::make_series({1.0, 2.0, 3.0, 100.0, nan}, {5}, f64).reshape({5, 1});

  cls::prep::SimpleImputer mean_imputer;  // strategy = Mean by default
  mean_imputer.learn(train);
  std::cout << "Mean:   " << mean_imputer.map(train).value() << "\n";

  cls::prep::SimpleImputerOptions median_opts{};
  median_opts.strategy = cls::prep::SimpleImputerStrategy::Median;
  cls::prep::SimpleImputer median_imputer(median_opts);
  median_imputer.learn(train);
  std::cout << "Median: " << median_imputer.map(train).value() << "\n";

  return 0;
}
```

Output

```text
Mean:   pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[1],
  [2],
  [3],
  [100],
  [26.5]]
Median: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[1],
  [2],
  [3],
  [100],
  [2.5]]
```

### Example 2

`MostFrequent` on a categorical-style column, and `Constant` filling every
missing entry with a fixed sentinel regardless of the data.

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
  train.x = pluss::make_series({1.0, 1.0, 2.0, nan}, {4}, f64).reshape({4, 1});

  cls::prep::SimpleImputerOptions freq_opts{};
  freq_opts.strategy = cls::prep::SimpleImputerStrategy::MostFrequent;
  cls::prep::SimpleImputer freq_imputer(freq_opts);
  freq_imputer.learn(train);
  std::cout << "MostFrequent: " << freq_imputer.map(train).value() << "\n";

  cls::prep::SimpleImputerOptions const_opts{};
  const_opts.strategy = cls::prep::SimpleImputerStrategy::Constant;
  const_opts.constant_value = -1.0;
  cls::prep::SimpleImputer const_imputer(const_opts);
  const_imputer.learn(train);
  std::cout << "Constant(-1): " << const_imputer.map(train).value() << "\n";

  return 0;
}
```

Output

```text
MostFrequent: pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [1],
  [2],
  [1]]
Constant(-1): pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [1],
  [2],
  [-1]]
```

### Example 3

A feature that is entirely missing in the training batch fits
`empty_feature_fallback` instead of a real statistic, regardless of
`strategy`.

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
  train.x = pluss::make_series({nan, nan, nan}, {3}, f64).reshape({3, 1});

  cls::prep::SimpleImputerOptions options{};
  options.strategy = cls::prep::SimpleImputerStrategy::Mean;
  options.empty_feature_fallback = 42.0;
  cls::prep::SimpleImputer imputer(options);
  imputer.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({nan}, {1}, f64).reshape({1, 1});
  std::cout << "entirely-missing feature -> " << imputer.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
entirely-missing feature -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[42]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `strategy` (`Mean`/`Median`/
  `MostFrequent`/`Constant`), `constant_value`/`empty_feature_fallback`,
  `dump_state`/`load_state`.

## See Also

- [`SimpleImputerOptions`](SimpleImputerOptions.md) — the options struct,
  including the `SimpleImputerStrategy` enum.
- [`NeighborImputer`](NeighborImputer.md) / [`IterativeImputer`](IterativeImputer.md) —
  the two imputers that use *other* feature values, not just a feature's own
  statistic.
- [`imputation/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.
- [`Flow::add_simple_imputer`](../../compose/Flow/add_stage.md) — routing
  this into a `compose::Flow`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SimpleImputerOptions
```
<!-- apiref:toctree:end -->
