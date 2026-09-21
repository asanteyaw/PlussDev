# `pluss::ai::classic::prep::QuantileMap`

Replace each value with its fitted **empirical percentile** — where it
ranks among the training values for that feature, from `0` to `1` — output
either directly as that percentile (`Uniform`) or passed through the
inverse normal CDF to land on a standard-normal scale (`Gaussian`). Unlike
[`PowerMap`](PowerMap.md), which fits one smooth analytic function per
feature, this flattens the *entire empirical distribution* to the target
shape exactly (by construction, on the training data) regardless of how
irregular it is — at the cost of only being able to interpolate within the
range of values `learn` actually saw.

## Signature

```cpp
explicit classic::prep::QuantileMap(classic::prep::QuantileMapOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`QuantileMapOptions`](QuantileMapOptions.md).

## State

The fitted, sorted array of each feature's non-`NaN` training values lives
inside the object, set by `learn`, read back only through `map` /
`dump_state`. `trained()` reports whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature, sorts its non-`NaN` training values and retains them —
  this sorted array **is** the fitted empirical CDF; there is no smaller
  summary statistic to compute. A feature with zero non-`NaN` values fits
  an empty array (see `map`). Fails with `Status::invalid_argument` if
  `clip` is outside `[0, 0.5)`.

`map(batch, options = {})`
: For each value, computes its rank within the fitted sorted array — tied
  values share the average of their ranks, matching
  [`RobustScaler`](../scaling/RobustScaler.md)'s own linear-interpolated
  quantile convention in spirit — divides by the fitted count to get a
  probability in `[0, 1]`, then clamps it to `[clip, 1 - clip]` before
  producing output: the clamped probability itself if `output = Uniform`,
  or that probability passed through the inverse normal CDF if `output =
  Gaussian`. A feature with an empty fitted array (nothing was fit) maps
  every value to `NaN`. `Status::precondition` if not yet trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, sizes, flat_sorted_values]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `clip` outside `[0, 0.5)` — `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The default `Uniform` output's exact rank-based percentiles, `Gaussian`
output on the same data, and `clip` visibly bounding the output away from
the extremes.

### Example 1

Five ranked training values mapped back onto themselves — each lands at
its own exact percentile.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::prep::QuantileMap map;  // output = Uniform by default
  const pluss::Series percentiles = map.learn_map(train).value();
  std::cout << "percentiles = " << percentiles << "\n";

  return 0;
}
```

Output

```text
percentiles = pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[0.1],
  [0.3],
  [0.5],
  [0.7],
  [0.9]]
```

### Example 2

`output = Gaussian` maps the same fitted ranks through the inverse normal
CDF — the middle value lands at (approximately) `0`, the standard-normal
median.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::prep::QuantileMapOptions options{};
  options.output = cls::prep::QuantileMapOutput::Gaussian;
  cls::prep::QuantileMap map(options);
  std::cout << "Gaussian output = " << map.learn_map(train).value() << "\n";

  return 0;
}
```

Output

```text
Gaussian output = pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[-1.2815516],
  [-0.52440051],
  [0],
  [0.52440051],
  [1.2815516]]
```

### Example 3

A value beyond the fitted range is clamped to rank `1.0`, then further
clamped by `clip` — visibly different from the unclamped `1.0` a larger
`clip` would otherwise report.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1000.0}, {1}, f64).reshape({1, 1});  // far beyond the fitted range

  cls::prep::QuantileMapOptions tight_clip{};
  tight_clip.clip = 1e-7;  // the default
  cls::prep::QuantileMap default_map(tight_clip);
  default_map.learn(train);
  std::cout << "clip=1e-7: " << default_map.map(probe).value() << "\n";

  cls::prep::QuantileMapOptions loose_clip{};
  loose_clip.clip = 0.1;
  cls::prep::QuantileMap wide_map(loose_clip);
  wide_map.learn(train);
  std::cout << "clip=0.1:  " << wide_map.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
clip=1e-7: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0.9999999]]
clip=0.1:  pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0.9]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `output`/`clip`, `dump_state`/
  `load_state`.

## See Also

- [`QuantileMapOptions`](QuantileMapOptions.md) — the options struct.
- [`PowerMap`](PowerMap.md) — a smooth analytic alternative for the same
  "reshape a skewed distribution" problem.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

QuantileMapOptions
```
<!-- apiref:toctree:end -->
