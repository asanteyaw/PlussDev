# `pluss::ai::classic::prep::MinMaxScaler`

Affinely map each feature's fitted `[min, max]` range onto a fixed output
interval, `[lower, upper]` (default `[0, 1]`):
`(value - min) / (max - min) * (upper - lower) + lower`. Reach for this
instead of [`StandardScaler`](StandardScaler.md) when a downstream step needs
input confined to a *known, bounded* interval — a sigmoid activation, a
distance metric that assumes `[0, 1]` inputs, or a visualization axis — rather
than merely a comparable scale. Unlike `StandardScaler`, the output range is
guaranteed only when `map` is called on data within the fitted `[min, max]`;
new data outside that range maps outside `[lower, upper]` too, unless you set
`clip`.

## Signature

```cpp
explicit classic::prep::MinMaxScaler(classic::prep::MinMaxScalerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`MinMaxScalerOptions`](MinMaxScalerOptions.md).

## State

The fitted per-feature minimum and maximum live inside the object, set by
`learn`, read back only through `map` / `dump_state`. `trained()` reports
whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature column, records the minimum and maximum over the column's
  non-`NaN` values. A feature with no non-`NaN` values at all fits `min = 0`,
  `max = 1` (a placeholder pair that only matters if `map` is later given a
  non-`NaN` value for that column, which contradicts having fit on an
  all-missing column in the first place). Returns `Status::invalid_argument`
  first if `lower >= upper` or either bound is non-finite, or if `range_floor`
  is not a positive finite number — checked before any data is read.

`map(batch, options = {})`
: Applies the affine map above using the fitted `min`/`max`. If the fitted
  range (`max - min`) does not exceed `range_floor` in absolute value, every
  value for that feature maps to `lower` instead of dividing (see
  [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback)).
  When `clip` (the scaler's own construction-time option, not `InferOptions`)
  is `true`, the mapped value is then clamped into
  `[lower, upper]` — this is the only scaler among the four with an optional
  clamp, since it is the only one with a fixed target interval to clamp
  into. `Status::precondition` if not yet trained; `NaN` passes through
  unchanged.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, data_min, data_max]`, following
  [`scaling/Introduction`](Introduction.md#persistence).

## Errors

- `lower >= upper`, or either non-finite — `Status::invalid_argument` from
  `learn`.
- `range_floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument` from
  `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn` was
  fit on — `Status::shape_mismatch`.

## Practical Examples

The default `[0, 1]` mapping, a custom output interval, `clip` containing
data that falls outside the fitted range, and the degenerate (constant
feature) fallback.

### Example 1

The default construction: fitted `[min, max]` mapped onto `[0, 1]`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature range is [10, 30].
  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::MinMaxScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "map([10, 20, 30]) over fitted [10, 30] -> [0, 1] = "
            << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([10, 20, 30]) over fitted [10, 30] -> [0, 1] = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [0.5],
  [1]]
```

### Example 2

`lower` / `upper` set to `[-1, 1]` — the interval a `tanh`-shaped model stage
often expects.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::MinMaxScalerOptions options{};
  options.lower = -1.0;
  options.upper = 1.0;
  cls::prep::MinMaxScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "map([10, 20, 30]) over fitted [10, 30] -> [-1, 1] = "
            << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([10, 20, 30]) over fitted [10, 30] -> [-1, 1] = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-1],
  [0],
  [1]]
```

### Example 3

`clip = true` versus the default `false`, on a `map` batch that reaches
outside the fitted training range.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0, 40.0}, {2}, f64).reshape({2, 1});  // outside [10, 30]

  cls::prep::MinMaxScaler unclipped;
  unclipped.learn(train);
  std::cout << "clip=false, map([0, 40]) = " << unclipped.map(probe).value() << "\n";

  cls::prep::MinMaxScalerOptions options{};
  options.clip = true;
  cls::prep::MinMaxScaler clipped(options);
  clipped.learn(train);
  std::cout << "clip=true,  map([0, 40]) = " << clipped.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
clip=false, map([0, 40]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-0.5],
  [1.5]]
clip=true,  map([0, 40]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0],
  [1]]
```

### Example 4

A constant feature falls back to `lower` for every value instead of dividing
by a zero range.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({5.0, 5.0, 5.0}, {3}, f64).reshape({3, 1});  // constant

  cls::prep::MinMaxScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0, 100.0}, {2}, f64).reshape({2, 1});
  std::cout << "constant feature, map([5, 100]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
constant feature, map([5, 100]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0],
  [0]]
```

## Changelog

- **0.1** — `learn` / `map` / `learn_map`, `lower` / `upper` / `clip` /
  `range_floor`, `dump_state` / `load_state`.

## See Also

- [`MinMaxScalerOptions`](MinMaxScalerOptions.md) — the options struct.
- [`StandardScaler`](StandardScaler.md) / [`RobustScaler`](RobustScaler.md) /
  [`MaxAbsScaler`](MaxAbsScaler.md) — the other three scalers.
- [`scaling/Introduction`](Introduction.md) — the contract shared by all four.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MinMaxScalerOptions
```
<!-- apiref:toctree:end -->
