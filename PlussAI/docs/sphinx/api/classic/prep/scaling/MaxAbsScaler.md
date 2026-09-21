# `pluss::ai::classic::prep::MaxAbsScaler`

Divide each feature by the largest absolute value seen during `learn` —
`value / max(|value|)` — with no centering step at all. The result lands in
`[-1, 1]`, and critically, **a zero input value stays exactly zero**: unlike
every other scaler here, `MaxAbsScaler` never shifts a column, so it is the
one to reach for when a feature is sparse (mostly zeros) and you need that
sparsity preserved, or when a feature is already meaningfully centered at
zero (a signed quantity) and only its magnitude needs taming.

## Signature

```cpp
explicit classic::prep::MaxAbsScaler(classic::prep::MaxAbsScalerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`MaxAbsScalerOptions`](MaxAbsScalerOptions.md).

## State

The fitted per-feature maximum absolute value lives inside the object, set by
`learn`, read back only through `map` / `dump_state`. `trained()` reports
whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature column, computes `max(|value|)` over the column's
  non-`NaN` values. If that maximum does not reach `floor` (this includes an
  all-zero column, an entirely-missing column, or any column whose largest
  magnitude is smaller than `floor`), the fitted scale for that feature falls
  back to `1` instead of dividing by (near) zero (see
  [`scaling/Introduction`](Introduction.md#degenerate-feature-fallback)).
  Returns `Status::invalid_argument` first if `floor` is not a positive
  finite number.

`map(batch, options = {})`
: Applies `value / scale` using the fitted maximum. `Status::precondition` if
  not yet trained; `NaN` passes through unchanged. There is no centering
  step and no separate toggle to disable scaling — `MaxAbsScaler` always
  divides.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, scale]` — one fitted array rather than the `[metadata, center,
  scale]` shape the other three scalers use, since `MaxAbsScaler` never fits
  a center. See [`scaling/Introduction`](Introduction.md#persistence).

## Errors

- `floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument` from
  `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn` was
  fit on — `Status::shape_mismatch`.

## Practical Examples

The default fit, a signed feature to show the no-centering, sign-preserving
behavior, a sparse (mostly-zero) feature to show zeros stay exactly zero, and
the all-zero degenerate fallback.

### Example 1

The default fit and the `[-1, 1]` range it produces.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, -8.0, 4.0}, {3}, f64).reshape({3, 1});  // max |value| = 8

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({8.0, -8.0, 4.0, 0.0}, {4}, f64).reshape({4, 1});
  std::cout << "max|value| = 8, map([8, -8, 4, 0]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
max|value| = 8, map([8, -8, 4, 0]) = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [-1],
  [0.5],
  [0]]
```

### Example 2

A signed feature: no centering means the sign of every value is preserved,
unlike a scaler that would shift the column first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // a signed residual feature, asymmetric around zero.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);
  const pluss::Series scaled = scaler.map(train).value();

  std::cout << "map(train) = " << scaled << "\n";

  return 0;
}
```

Output

```text
map(train) = pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[-0.4],
  [-0.2],
  [0],
  [0.2],
  [1]]
```

### Example 3

A sparse feature (mostly zeros): every zero entry maps to exactly zero,
preserving sparsity — the property `MaxAbsScaler` exists to guarantee.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({0.0, 0.0, 0.0, 40.0, 0.0}, {5}, f64).reshape({5, 1});

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);
  const pluss::Series scaled = scaler.map(train).value();

  std::cout << "map(train) = " << scaled << "\n";

  return 0;
}
```

Output

```text
map(train) = pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[0],
  [0],
  [0],
  [1],
  [0]]
```

### Example 4

An all-zero feature falls back to `scale = 1` — every value (already zero)
stays zero, rather than dividing zero by zero.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64).reshape({3, 1});  // all-zero

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0}, {1}, f64).reshape({1, 1});
  std::cout << "all-zero feature, map([0]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
all-zero feature, map([0]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
```

## Changelog

- **0.1** — `learn` / `map` / `learn_map`, `floor`, `dump_state` /
  `load_state`.

## See Also

- [`MaxAbsScalerOptions`](MaxAbsScalerOptions.md) — the options struct.
- [`StandardScaler`](StandardScaler.md) / [`MinMaxScaler`](MinMaxScaler.md) /
  [`RobustScaler`](RobustScaler.md) — the other three scalers.
- [`scaling/Introduction`](Introduction.md) — the contract shared by all four.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MaxAbsScalerOptions
```
<!-- apiref:toctree:end -->
