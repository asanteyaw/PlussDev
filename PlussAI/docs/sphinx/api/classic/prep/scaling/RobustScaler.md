# `pluss::ai::classic::prep::RobustScaler`

Center and scale each feature using statistics that resist outliers: the
median instead of the mean, and the interquartile range (IQR, by default the
75th minus the 25th percentile) instead of the standard deviation —
`(value - median) / IQR`. A handful of extreme values barely move a median or
an IQR, while they can shift a mean and inflate a standard deviation
substantially; reach for `RobustScaler` instead of
[`StandardScaler`](StandardScaler.md) whenever a column has outliers you want
the *scaling* to ignore (as opposed to [`OutlierIsolator`](../outlier/OutlierIsolator.md),
which flags or removes the rows outright).

## Signature

```cpp
explicit classic::prep::RobustScaler(classic::prep::RobustScalerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`RobustScalerOptions`](RobustScalerOptions.md).

## State

The fitted per-feature median (or `0`, if `with_centering = false`) and IQR
(or `1`, if `with_scaling = false`) live inside the object, set by `learn`,
read back only through `map` / `dump_state`. `trained()` reports whether
`learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature column, computes three quantiles over the column's
  non-`NaN` values, using linear interpolation between the two nearest ranked
  values (the same convention as `RobustScaler`'s own `lower_quantile` /
  `upper_quantile`, applied here to the `0.5` quantile too): the median, the
  `lower_quantile` value, and the `upper_quantile` value. The center is the
  median (`with_centering ? median : 0`); the scale is
  `upper_quantile value - lower_quantile value`
  (`with_scaling ? iqr : 1`). A feature with no non-`NaN` values fits
  `median = 0`, `iqr = 1`. If the fitted IQR does not exceed `range_floor` in
  absolute value, the scale falls back to `1` instead (see
  [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback)).
  Returns `Status::invalid_argument` first if `lower_quantile` /
  `upper_quantile` are not both within `[0, 1]` with `lower_quantile <
  upper_quantile`, or if `range_floor` is not a positive finite number.

`map(batch, options = {})`
: Applies `(value - center) / scale` using the fitted numbers.
  `Status::precondition` if not yet trained; `NaN` passes through unchanged.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, center, scale]`, following
  [`scaling/Introduction`](Introduction.md#persistence).

## Errors

- `lower_quantile` / `upper_quantile` outside `[0, 1]`, or
  `lower_quantile >= upper_quantile` — `Status::invalid_argument` from
  `learn`.
- `range_floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument` from
  `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn` was
  fit on — `Status::shape_mismatch`.

## Practical Examples

The default `[0.25, 0.75]` IQR fit, a side-by-side comparison against
`StandardScaler` on data with one outlier (the behavior this scaler exists
for), a widened quantile spread, and the degenerate (constant feature)
fallback.

### Example 1

The default fit: median and IQR over eight ranked values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8}, f64).reshape({8, 1});

  cls::prep::RobustScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({4.5, 8.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([4.5, 8.0]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([4.5, 8.0]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0],
  [1]]
```

### Example 2

`RobustScaler` versus `StandardScaler` on data with one extreme outlier: the
robust fit's median/IQR barely move, the standard fit's mean/stddev are
pulled hard by the outlier.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // five ordinary points clustered around 10, then one wild outlier.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({9.0, 10.0, 10.0, 11.0, 10.0, 1000.0}, {6}, f64).reshape({6, 1});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});  // a typical, non-outlier value

  cls::prep::RobustScaler robust;
  robust.learn(train);
  cls::prep::StandardScaler standard;
  standard.learn(train);

  std::cout << "RobustScaler::map(10)   = " << robust.map(probe).value() << "\n";
  std::cout << "StandardScaler::map(10) = " << standard.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
RobustScaler::map(10)   = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
StandardScaler::map(10) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[-0.44721305]]
```

### Example 3

Widening the quantile spread to `[0.1, 0.9]` — a smaller `IQR`-equivalent
than the default `[0.25, 0.75]` would give here, since more of the data now
falls inside the bracket.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});

  cls::prep::RobustScalerOptions options{};
  options.lower_quantile = 0.1;
  options.upper_quantile = 0.9;
  cls::prep::RobustScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});
  std::cout << "quantiles=[0.1, 0.9], map([10]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
quantiles=[0.1, 0.9], map([10]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0.625]]
```

### Example 4

A constant feature falls back to `scale = 1` instead of dividing by a zero
IQR.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({3.0, 3.0, 3.0, 3.0}, {4}, f64).reshape({4, 1});  // constant

  cls::prep::RobustScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({3.0, 50.0}, {2}, f64).reshape({2, 1});
  std::cout << "constant feature, map([3, 50]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
constant feature, map([3, 50]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0],
  [47]]
```

## Changelog

- **0.1** — `learn` / `map` / `learn_map`, `with_centering` / `with_scaling` /
  `lower_quantile` / `upper_quantile` / `range_floor`, `dump_state` /
  `load_state`.

## See Also

- [`RobustScalerOptions`](RobustScalerOptions.md) — the options struct.
- [`StandardScaler`](StandardScaler.md) / [`MinMaxScaler`](MinMaxScaler.md) /
  [`MaxAbsScaler`](MaxAbsScaler.md) — the other three scalers.
- [`OutlierIsolator`](../outlier/OutlierIsolator.md) — flags or scores
  outlier rows outright, rather than de-emphasizing them during scaling.
- [`scaling/Introduction`](Introduction.md) — the contract shared by all four.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

RobustScalerOptions
```
<!-- apiref:toctree:end -->
