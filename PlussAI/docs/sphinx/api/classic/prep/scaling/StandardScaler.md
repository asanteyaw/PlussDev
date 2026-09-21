# `pluss::ai::classic::prep::StandardScaler`

Center and scale each feature to (approximately) zero mean and unit variance:
`(value - mean) / stddev`, fit per feature from the training batch. This is
the default scaler to reach for — most distance-based (k-means, k-NN),
gradient-based (linear/logistic regression, neural nets), and
variance-based (PCA) algorithms implicitly assume input on a comparable,
roughly-unit scale, and `StandardScaler` is the standard way to get there.
Prefer [`RobustScaler`](RobustScaler.md) instead when the column has outliers
a mean/stddev fit would be distorted by.

## Signature

```cpp
explicit classic::prep::StandardScaler(classic::prep::StandardScalerOptions options = {});
```

## Construction

`StandardScaler` is a concrete value type, constructed directly — there is no
factory function. Pass a [`StandardScalerOptions`](StandardScalerOptions.md)
to change the defaults, or omit it to center and scale every feature with the
population statistics.

## State

The fitted center and scale live inside the object, one pair per feature, set
by `learn` and read back only through `map` / `dump_state` — there is no
public accessor for the raw numbers. `trained()` reports whether `learn` has
succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature column, computes the mean (`with_centering ? mean : 0`) and
  the **population** standard deviation (`with_scaling ? stddev : 1`) over the
  column's non-`NaN` values — `NaN` entries are excluded, not treated as
  `0`. `stddev` uses divisor `n` (not `n - 1`): the biased/population
  estimator, matching every other classic prep kernel that reports a spread.
  If the computed `stddev` is not finite or does not exceed
  `options.variance_floor`, the fitted scale for that feature falls back to
  `1` instead (see [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback)).
  Returns `Status::invalid_argument` first if `variance_floor` itself is not a
  positive finite number, before reading any data. `options` (`TrainOptions`)
  is accepted for interface symmetry with every other `TransformerBase`, but
  is not consulted by `StandardScaler`.

`map(batch, options = {})`
: Applies `(value - center) / scale` per feature using the fitted numbers.
  `Status::precondition` if `learn` has not yet succeeded, `Status::shape_mismatch`
  if `batch`'s layout or feature count differs from what `learn` was fit on.
  `NaN` entries pass through unchanged. `options` (`InferOptions`) is accepted
  for interface symmetry, not consulted.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn(batch, learn_options)` then `map(batch, infer_options)` — the fitted
  representation of the same data you fit on, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, center, scale]`, following the pattern documented in
  [`scaling/Introduction`](Introduction.md#persistence).

## Errors

- `variance_floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument`
  from `learn`, checked before any data is read.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn` was
  fit on — `Status::shape_mismatch`.
- An undefined or rank-1 `FeatureBatch.x` — the same `Status` that
  [`validate_features`](../../data/FeatureBatch.md) reports directly.

## Practical Examples

The default fit on a two-feature batch, then each of the three behavior
toggles (`with_centering`, `with_scaling`, and the `variance_floor` fallback)
in isolation, plus `NaN` passthrough, and the shared
`learn`/`dump_state`/`load_state` persistence round trip.

### Example 1

The default construction: both centering and scaling active, verified against
the fitted mean and population standard deviation.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // One feature column: values 2, 4, 4, 4, 5, 5, 7, 9 -- mean 5, population stddev 2.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}, {8}, f64).reshape({8, 1});

  cls::prep::StandardScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0, 9.0, 1.0}, {3}, f64).reshape({3, 1});
  const pluss::Series scaled = scaler.map(probe).value();

  std::cout << "mean = 5, stddev = 2\n";
  std::cout << "map([5, 9, 1]) = " << scaled << "\n";

  return 0;
}
```

Output

```text
mean = 5, stddev = 2
map([5, 9, 1]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [2],
  [-2]]
```

### Example 2

`with_centering = false`: only divide by the standard deviation, leave the
mean untouched. Useful when a feature is already meaningfully centered (e.g.
a signed residual) and shifting it would discard that meaning.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}, {8}, f64).reshape({8, 1});

  cls::prep::StandardScalerOptions options{};
  options.with_centering = false;
  cls::prep::StandardScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});
  std::cout << "with_centering=false, map([10]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
with_centering=false, map([10]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[5]]
```

### Example 3

`with_scaling = false`: only subtract the mean, leave the spread untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}, {8}, f64).reshape({8, 1});

  cls::prep::StandardScalerOptions options{};
  options.with_scaling = false;
  cls::prep::StandardScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});
  std::cout << "with_scaling=false, map([10]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
with_scaling=false, map([10]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[5]]
```

### Example 4

A constant feature (zero variance) falls back to `scale = 1` instead of
dividing by zero, and a `NaN` entry passes through `map` untouched.

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

  // feature 0 is constant (every value 7); feature 1 has real variance and one missing entry.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({7.0, 1.0, 7.0, nan, 7.0, 7.0}, {6}, f64).reshape({3, 2});

  cls::prep::StandardScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({7.0, nan, 10.0, 3.0}, {4}, f64).reshape({2, 2});
  const pluss::Series scaled = scaler.map(probe).value();

  std::cout << "constant feature 0, missing probe entry: map = " << scaled << "\n";

  return 0;
}
```

Output

```text
constant feature 0, missing probe entry: map = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, nan],
  [3, -0.33333333]]
```

### Example 5

The shared failure mode every scaler in this family has — `map` before
`learn` — and a `learn` / `dump_state` / `load_state` round trip onto a
freshly constructed scaler, verifying the reloaded instance maps identically
to the original.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::StandardScaler fresh;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  const cls::StatusOr<pluss::Series> too_early = fresh.map(probe);
  std::cout << "map before learn: ok = " << std::boolalpha << too_early.ok()
            << ", message = \"" << too_early.status().message << "\"\n";

  cls::FeatureBatch train{};
  train.x = pluss::make_series({-1.0, 9.0, 1.0, 11.0}, {4}, f64).reshape({2, 2});
  cls::prep::StandardScaler original;
  original.learn(train);
  cls::FeatureBatch minus_one_sigma{};
  minus_one_sigma.x = pluss::make_series({-1.0, 9.0}, {2}, f64).reshape({1, 2});  // == row 0, exactly 1 stddev below each mean
  const pluss::Series original_out = original.map(minus_one_sigma).value();

  const std::vector<pluss::Series> state = original.dump_state().value();
  cls::prep::StandardScaler reloaded;
  std::cout << "trained() before load_state = " << std::boolalpha << reloaded.trained() << "\n";
  reloaded.load_state(state);
  std::cout << "trained() after load_state  = " << reloaded.trained() << "\n";

  const pluss::Series reloaded_out = reloaded.map(minus_one_sigma).value();
  std::cout << "original.map(-1 stddev row) = " << original_out << "\n";
  std::cout << "reloaded.map(-1 stddev row) = " << reloaded_out << "\n";

  return 0;
}
```

Output

```text
map before learn: ok = false, message = "StandardScaler::map requires learn() before map()."
trained() before load_state = false
trained() after load_state  = true
original.map(-1 stddev row) = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-1, -1]]
reloaded.map(-1 stddev row) = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-1, -1]]
```

## Changelog

- **0.1** — `learn` / `map` / `learn_map`, `with_centering` / `with_scaling` /
  `variance_floor`, `dump_state` / `load_state`.

## See Also

- [`StandardScalerOptions`](StandardScalerOptions.md) — the options struct.
- [`MinMaxScaler`](MinMaxScaler.md) / [`RobustScaler`](RobustScaler.md) /
  [`MaxAbsScaler`](MaxAbsScaler.md) — the other three scalers.
- [`scaling/Introduction`](Introduction.md) — the contract shared by all four.
- [`Flow::add_standard_scaler`](../../compose/Flow/add_stage.md) — routing
  this into a `compose::Flow`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

StandardScalerOptions
```
<!-- apiref:toctree:end -->
