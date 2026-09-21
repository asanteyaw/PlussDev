# `pluss::ai::classic::prep::PowerMap`

Fit a Yeo-Johnson or Box-Cox power transform per feature — a single
parameter λ chosen (by a grid search maximizing the transform's profile
log-likelihood, unless you supply λ yourself) to make the transformed
distribution as close to symmetric/normal as a one-parameter power family
can get, then optionally standardize the transformed values to zero mean
and unit variance. Reach for this over [`QuantileMap`](QuantileMap.md) when
a smooth, invertible-in-spirit analytic transform is preferable to an
exact-but-only-empirical percentile mapping — the usual reason being a
skewed feature that a model downstream assumes is roughly normal.

## Signature

```cpp
explicit classic::prep::PowerMap(classic::prep::PowerMapOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`PowerMapOptions`](PowerMapOptions.md).

## State

The fitted per-feature λ, and (when `standardize` is on) the post-transform
mean and standard deviation, live inside the object, set by `learn`, read
back only through `map` / `dump_state`. `trained()` reports whether `learn`
has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature, if `lambda` was supplied explicitly, uses it directly;
  otherwise searches `lambda_grid` evenly-spaced candidates between
  `lambda_min` and `lambda_max`, picking whichever maximizes the profile
  log-likelihood for the chosen `mode`. `mode = BoxCox` additionally
  requires every non-`NaN` training (and, later, `map`) value to be
  **strictly positive** — Box-Cox is undefined otherwise; `mode =
  YeoJohnson` has no such requirement (it branches internally on the
  value's own sign, which is the whole reason it exists as an alternative).
  If `standardize` is on, also fits the post-transform mean and standard
  deviation the same way [`StandardScaler`](../scaling/StandardScaler.md)
  does, including its floor-guarded degenerate-variance fallback.

`map(batch, options = {})`
: Applies the fitted power transform with that feature's λ, then
  (`standardize`) the fitted `(y - mean) / stddev`. `Status::precondition`
  if not yet trained; `Status::invalid_argument` if `mode = BoxCox` and a
  non-positive value appears, even if the feature never saw one during
  `learn`.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, lambda, center, scale]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence) —
  `center`/`scale` are all-zero/all-one when `standardize` was off at fit
  time.

## Errors

- `variance_floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument`
  from `learn`.
- `lambda_grid < 2` — `Status::invalid_argument` from `learn`.
- `lambda_min`/`lambda_max` non-finite, or not `lambda_min < lambda_max` —
  `Status::invalid_argument` from `learn`.
- `lambda` supplied but non-finite — `Status::invalid_argument` from
  `learn`.
- `mode = BoxCox` and a non-positive value appears — `Status::invalid_argument`
  from `learn` (training data) or `map` (new data), independently.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.

## Practical Examples

`YeoJohnson` fitting a positively-skewed feature (visibly more symmetric
afterward), `BoxCox` rejecting non-positive data, and an explicit `lambda`
bypassing the grid search entirely.

### Example 1

An exponentially-growing feature — heavily right-skewed — reshaped by a
fitted Yeo-Johnson transform.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0}, {7}, f64).reshape({7, 1});

  cls::prep::PowerMap power_map;  // YeoJohnson, standardize = true by default
  const pluss::Series reshaped = power_map.learn_map(train).value();
  std::cout << "reshaped = " << reshaped << "\n";

  return 0;
}
```

Output

```text
reshaped = pluss::Series(shape=[7,1], strides=[1,1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[[-1.4282765],
  [-1.0302276],
  [-0.55118288],
  [-0.02940899],
  [0.50165736],
  [1.0206945],
  [1.5167441]]
```

### Example 2

`mode = BoxCox` on data that includes a non-positive value — rejected
outright, versus `YeoJohnson` succeeding on the exact same batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({-1.0, 1.0, 2.0, 4.0}, {4}, f64).reshape({4, 1});

  cls::prep::PowerMapOptions box_cox_opts{};
  box_cox_opts.mode = cls::prep::PowerMapMode::BoxCox;
  cls::prep::PowerMap box_cox(box_cox_opts);
  const cls::Status box_cox_fit = box_cox.learn(train);
  std::cout << "BoxCox ok = " << std::boolalpha << box_cox_fit.ok() << "\n";
  std::cout << "message = \"" << box_cox_fit.message << "\"\n";

  cls::prep::PowerMap yeo_johnson;  // YeoJohnson is the default mode
  const cls::Status yeo_fit = yeo_johnson.learn(train);
  std::cout << "YeoJohnson ok = " << yeo_fit.ok() << "\n";

  return 0;
}
```

Output

```text
BoxCox ok = false
message = "PowerMap(BoxCox) requires strictly positive observed values."
YeoJohnson ok = true
```

### Example 3

Supplying `lambda` explicitly skips the grid search entirely — the
Box-Cox formula `(x^λ - 1) / λ` at λ = `1` reduces to `x - 1`, a plain
shift, verified directly.

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

  cls::prep::PowerMapOptions options{};
  options.mode = cls::prep::PowerMapMode::BoxCox;
  options.standardize = false;  // isolate the power transform itself
  options.lambda = 1.0;
  cls::prep::PowerMap power_map(options);
  std::cout << "lambda=1 (BoxCox, no standardize) = " << power_map.learn_map(train).value() << "\n";

  return 0;
}
```

Output

```text
lambda=1 (BoxCox, no standardize) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [1],
  [2]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `mode`/`standardize`/`lambda`/
  `lambda_min`/`lambda_max`/`lambda_grid`/`variance_floor`,
  `dump_state`/`load_state`.

## See Also

- [`PowerMapOptions`](PowerMapOptions.md) — the options struct.
- [`QuantileMap`](QuantileMap.md) — the empirical-percentile alternative.
- [`StandardScaler`](../scaling/StandardScaler.md) — the standardization
  step this type applies after its own power transform.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

PowerMapOptions
```
<!-- apiref:toctree:end -->
