# `pluss::ai::classic::prep::ThresholdBinarizer`

Map every value to one of exactly two fixed outputs, by a threshold:
strictly greater than `threshold` becomes `high_value`, everything else
(including a value exactly equal to `threshold`) becomes `low_value`. The
simplest transform in this sub-family — no vocabulary or statistic is
fitted from data at all — for turning a continuous feature into a hard
yes/no signal at a boundary you already know.

## Signature

```cpp
explicit classic::prep::ThresholdBinarizer(classic::prep::ThresholdBinarizerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`ThresholdBinarizerOptions`](ThresholdBinarizerOptions.md).

## State

`learn` fits nothing but the feature count and layout — `threshold`,
`low_value`, and `high_value` are fixed at construction, not derived from
the training data. `trained()` reports whether `learn` has succeeded yet,
needed only to validate the shape `map` later receives.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Records `feature_count()` and `layout` for later shape validation. Fails
  with `Status::invalid_argument` if `threshold`, `low_value`, or
  `high_value` is not finite. Does not read the batch's actual values.

`map(batch, options = {})`
: For every non-`NaN` value: `value > threshold ? high_value : low_value`
  — note the **strict** inequality, so a value exactly at `threshold` maps
  to `low_value`, not `high_value`. `NaN` passes through unchanged.
  `Status::precondition` if not yet trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata]` — a single tensor (`trained`, `layout`, `feature_count`); no
  threshold is ever fitted from data.

## Errors

- `threshold`, `low_value`, or `high_value` not finite —
  `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The default `0`/`1` output around threshold `0`, a value exactly at the
threshold landing on the low side, and custom `low_value`/`high_value`.

### Example 1

The default: negative values become `0`, positive values become `1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-5.0, -0.001, 0.001, 5.0}, {4}, f64).reshape({4, 1});

  cls::prep::ThresholdBinarizer binarizer;  // threshold = 0
  std::cout << "map = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
map = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0],
  [0],
  [1],
  [1]]
```

### Example 2

A value exactly equal to `threshold` maps to `low_value` — the comparison
is strict.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::ThresholdBinarizerOptions options{};
  options.threshold = 10.0;
  cls::prep::ThresholdBinarizer binarizer(options);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({9.999, 10.0, 10.001}, {3}, f64).reshape({3, 1});
  std::cout << "map([9.999, 10.0 (== threshold), 10.001]) = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
map([9.999, 10.0 (== threshold), 10.001]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [0],
  [1]]
```

### Example 3

Custom `low_value`/`high_value` — a `-1`/`+1` signed output instead of the
default `0`/`1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::ThresholdBinarizerOptions options{};
  options.threshold = 0.0;
  options.low_value = -1.0;
  options.high_value = 1.0;
  cls::prep::ThresholdBinarizer binarizer(options);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-2.0, 2.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([-2, 2]) = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
map([-2, 2]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-1],
  [1]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `threshold`/`low_value`/
  `high_value`, `dump_state`/`load_state`.

## See Also

- [`ThresholdBinarizerOptions`](ThresholdBinarizerOptions.md) — the options
  struct.
- [`QuantileMap`](QuantileMap.md) — a data-driven alternative when the
  right threshold is a percentile rather than a value you already know.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

ThresholdBinarizerOptions
```
<!-- apiref:toctree:end -->
