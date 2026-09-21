# `pluss::ai::classic::prep::IterativeImputer`

Fill missing values with a round-robin ridge regression against every other
feature — a MICE-style ("multiple imputation by chained equations")
strategy. Every feature starts filled with its own mean; then, for
`rounds` passes, each feature in turn gets a ridge regression fit predicting
it from every other (currently-filled) feature, using only the rows that
were not originally missing for it, and its still-missing rows are
re-predicted from that fit — so later features in a round see the
already-updated values of earlier ones, and later rounds refine every
feature's fit using the previous round's improved fills. Reach for this
over [`SimpleImputer`](SimpleImputer.md) /
[`NeighborImputer`](NeighborImputer.md) when features are linearly
correlated enough that predicting one from the others is meaningfully more
accurate than either a fixed statistic or a neighbor average — at the cost
of being the most expensive of the three to fit, and the only one where
filling one feature depends on the (also filled) values of the others.

## Signature

```cpp
explicit classic::prep::IterativeImputer(classic::prep::IterativeImputerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`IterativeImputerOptions`](IterativeImputerOptions.md).

## State

The fitted per-feature ridge coefficients and intercepts, plus each
feature's mean (used as the round-zero fill and the fallback when a
feature could not be fit), live inside the object, set by `learn`, read
back only through `map` / `dump_state`. `trained()` reports whether `learn`
has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Fills every missing value with its feature's own mean (or
  `empty_feature_fallback` for a feature with zero non-`NaN` values) as a
  starting point, then — only if there is more than one feature; a
  single-feature batch stops here, fully mean-imputed — runs `rounds`
  passes: for each feature `f` in order, fits ridge regression coefficients
  (closed-form, an L2 penalty of `ridge` applied to every coefficient
  **except the intercept**) predicting `f` from every other feature, using
  only rows where `f` was not originally missing, then re-predicts every
  originally-missing row of `f` from that fit. A feature whose own values
  were all originally present is never fit (nothing missing to predict),
  though it still participates as a predictor for the others. Fails with
  `Status::invalid_argument` if `rounds <= 0`, or `ridge` is negative or
  non-finite.

`map(batch, options = {})`
: Fills every missing value with its feature's fitted mean, then — if
  `feature_count() > 1` — replays `rounds` passes of the fitted (not
  refit) coefficients over the originally-missing entries, in the same
  feature order `learn` used. `Status::precondition` if not yet trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, means, coefficients, intercepts]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `rounds <= 0` — `Status::invalid_argument` from `learn`.
- `ridge` negative or non-finite — `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The default fit on two linearly related features, `rounds` mattering only
once there are enough features for the chained regressions to actually
interact, and the single-feature short-circuit that leaves a plain
mean-fill.

### Example 1

Two features in an exact linear relationship (`y = 2x`) — the fitted ridge
regression recovers it closely enough to fill a missing value near the true
value, unlike a plain feature mean would.

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

  // y = 2x, one missing y.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 2.0, 2.0, 4.0, 3.0, 6.0, 4.0, nan}, {8}, f64).reshape({4, 2});

  cls::prep::IterativeImputerOptions options{};
  options.ridge = 1e-6;  // close to unregularized, to recover the exact relationship
  cls::prep::IterativeImputer imputer(options);
  const pluss::Series filled = imputer.learn_map(train).value();
  std::cout << "true y for x=4 is 8; filled = " << filled << "\n";

  return 0;
}
```

Output

```text
true y for x=4 is 8; filled = pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [2, 4],
  [3, 6],
  [4, 7.999998]]
```

### Example 2

A single-feature batch short-circuits to a plain mean fill — there is no
other feature to regress against.

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
  train.x = pluss::make_series({10.0, 20.0, 30.0, nan}, {4}, f64).reshape({4, 1});  // mean of 10,20,30 = 20

  cls::prep::IterativeImputer imputer;
  const pluss::Series filled = imputer.learn_map(train).value();
  std::cout << "single feature -> plain mean fill: " << filled << "\n";

  return 0;
}
```

Output

```text
single feature -> plain mean fill: pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[10],
  [20],
  [30],
  [20]]
```

### Example 3

`rounds = 1` versus a larger `rounds` on three mutually-informative
features with more than one missing value — more rounds let later
predictions benefit from earlier ones' refined fills within the same
`learn` call.

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

  // three features, all roughly equal to each other; two missing values in different rows.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 1.0, 1.0,
       2.0, 2.0, 2.0,
       3.0, nan, 3.0,
       4.0, 4.0, nan}, {12}, f64).reshape({4, 3});

  auto fit_and_fill = [&](std::int64_t rounds) {
    cls::prep::IterativeImputerOptions options{};
    options.rounds = rounds;
    options.ridge = 1e-6;
    cls::prep::IterativeImputer imputer(options);
    return imputer.learn_map(train).value();
  };

  std::cout << "rounds=1:  " << fit_and_fill(1) << "\n";
  std::cout << "rounds=10: " << fit_and_fill(10) << "\n";

  return 0;
}
```

Output

```text
rounds=1:  pluss::Series(shape=[4,3], strides=[3,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 1, 1],
  [2, 2, 2],
  [3, 3.000001, 3],
  [4, 4, 3.9999988]]
rounds=10: pluss::Series(shape=[4,3], strides=[3,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 1, 1],
  [2, 2, 2],
  [3, 3.0000001, 3],
  [4, 4, 3.9999994]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `rounds`/`ridge`/
  `empty_feature_fallback`, `dump_state`/`load_state`.

## See Also

- [`IterativeImputerOptions`](IterativeImputerOptions.md) — the options
  struct.
- [`SimpleImputer`](SimpleImputer.md) / [`NeighborImputer`](NeighborImputer.md) —
  the cheaper alternatives.
- [`imputation/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

IterativeImputerOptions
```
<!-- apiref:toctree:end -->
