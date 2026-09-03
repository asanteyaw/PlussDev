<!-- pluss-hand-authored -->

# `lymph::prep::stage::standard_scaler`

[Back to `prep::stage`](../stage_family.md)

`standard_scaler` builds an attachable [`prep::Stage`](Stage.md) that rescales the
routed numeric columns to zero mean and unit variance. It is the stage-shaped
counterpart of the standalone [`prep::StandardScaler`](../scaling/StandardScaler.md).

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::standard_scaler(
    lymph::options::StandardScaler options = {});
```

`lymph::options::StandardScaler` is the canonical spelling of the option struct;
`lymph::prep::StandardScalerOptions` is the family-scoped alias for the same type.

## What It Does

The call returns a `Stage` value — a descriptor, not a fitted object. It records
the transform kind (`StandardScaler`) and the options you pass. Nothing is
computed until the stage is paired with a [`prep::Route`](../route/Route.md) and
handed to [`Model::append`](../../models/Model/append.md),
[`Model::prepare`](../../models/Model/prepare.md), or a
`prep::FeatureTransformer` branch, which fits it once on training data and
replays the learned column mean and scale on every later frame.

For each routed column the fitted transform computes `(x - mean) / scale`, where
`mean` is the column mean and `scale` is the column standard deviation (with a
`variance_floor` guard against division by zero). Both steps are individually
switchable through the options.

### Option fields

The three fields of `lymph::options::StandardScaler`:

`bool with_centering = true`
: Subtract the column mean. Turn off to divide by the spread while leaving the
  column's location unchanged — useful when zero already means something in the
  data (Example 2).

`bool with_scaling = true`
: Divide by the column standard deviation. Turn off to centre on zero without
  changing the unit of measure (Example 3).

`double variance_floor = 1e-12`
: Lower bound on the divisor. A near-constant column has almost zero variance;
  the floor stops the scaled values from exploding. Rarely changed.

With both toggles off the transform is the identity map (Example 3).

The full field reference lives on the
[`options::StandardScaler` page](../scaling/StandardScalerOptions.md); it is the
same struct the standalone `prep::StandardScaler` takes.

## Parameters

`lymph::options::StandardScaler options`
: The scaler configuration. Defaulted, so `standard_scaler()` is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::StandardScaler` and the given
options. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A route that resolves to a non-numeric column is reported when the owning plan
  fits the stage, not here.
- A routed column with fewer than two distinct values fits without error but,
  with `with_scaling` on, contributes little signal after scaling.

## Practical Examples

### Example 1

The default stage, attached to a model. Two features on very different scales;
scaling puts them on equal footing before the estimator sees them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Two features on very different scales: an amount in the thousands and a
  // ratio near 1. Without scaling the amount dominates the fit.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage: zero-mean, unit-variance scaling on the routed columns.
  // No options argument means a default-constructed lymph::options::StandardScaler.
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count = 1
trained     = true

=======================================
     | amount  ratio  actual  predicted
---------------------------------------
0    | 1200    0.9    0       0        
1    | 1800    1.1    0       0        
2    | 3400    0.7    0       0        
3    | 4100    1.3    1       1        
4    | 5200    0.6    1       1        
5    | 6000    1.4    1       1        
---------------------------------------
[6 cs x 4 feat]
```

### Example 2

`with_centering`. The stage factory and the standalone `prep::StandardScaler`
take the same option struct; the standalone object shows the arithmetic
directly. With centering off, the scaled column keeps its sign and location.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series price = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const DataFrame frame({{"price", Thread(price)}});

  // with_centering toggles whether the column mean is subtracted. The stage
  // factory and the standalone prep::StandardScaler take the identical option
  // struct; here the standalone object makes the numeric effect visible.
  lymph::options::StandardScaler with_center{};                 // default: true
  lymph::options::StandardScaler spread_only{.with_centering = false};

  // A stage built from the same options is a valid descriptor either way.
  const lymph::prep::Stage staged = lymph::prep::stage::standard_scaler(spread_only);
  std::cout << "staged kind is StandardScaler: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::StandardScaler) << "\n\n";

  std::cout << "with_centering = true (mean removed, values straddle 0):\n";
  std::cout << lymph::prep::StandardScaler{with_center}.learn_map(frame) << "\n";

  std::cout << "with_centering = false (mean kept, values stay positive):\n";
  std::cout << lymph::prep::StandardScaler{spread_only}.learn_map(frame);

  return 0;
}
```

Output

```text
staged kind is StandardScaler: true

with_centering = true (mean removed, values straddle 0):
====================
     | price        
--------------------
0    | -1.414213562 
1    | -0.7071067812
2    |  0           
3    |  0.7071067812
4    |  1.414213562 
--------------------
[5 cs x 1 feat]
with_centering = false (mean kept, values stay positive):
===================
     | price       
-------------------
0    | 0.7071067812
1    | 1.414213562 
2    | 2.121320344 
3    | 2.828427125 
4    | 3.535533906 
-------------------
[5 cs x 1 feat]
```

### Example 3

`with_scaling`, and both toggles off. Family-alias spelling of the option struct
this time. Centering without scaling shifts the column to zero mean and keeps its
original units; both off is a pass-through.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series price = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const DataFrame frame({{"price", Thread(price)}});

  // with_scaling toggles the divide-by-spread step. Turn it off to centre the
  // column on zero without changing its unit of measure. Family-alias spelling
  // for the option struct this time.
  lymph::prep::StandardScalerOptions centre_only{};
  centre_only.with_scaling = false;

  std::cout << "with_scaling = false (mean removed, spread unchanged):\n";
  std::cout << lymph::prep::StandardScaler{centre_only}.learn_map(frame) << "\n";

  // Both toggles off is an identity map -- the column passes through untouched.
  lymph::prep::StandardScalerOptions identity{};
  identity.with_centering = false;
  identity.with_scaling = false;
  std::cout << "both toggles off (identity):\n";
  std::cout << lymph::prep::StandardScaler{identity}.learn_map(frame);

  return 0;
}
```

Output

```text
with_scaling = false (mean removed, spread unchanged):
============
     | price
------------
0    | -20  
1    | -10  
2    |  0   
3    |  10  
4    |  20  
------------
[5 cs x 1 feat]
both toggles off (identity):
============
     | price
------------
0    | 10   
1    | 20   
2    | 30   
3    | 40   
4    | 50   
------------
[5 cs x 1 feat]
```

### Example 4

One step among several. The scaler stage handles the two numeric columns while a
one-hot stage handles the categorical one, both passed in a single batch
`append`. Fully qualified factory spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
  });

  // A standard-scaler stage is one step among several. Here it scales the two
  // numeric columns while a one-hot stage handles the categorical one. The
  // fully qualified factory spelling: lymph::prep::stage::standard_scaler.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append({
      {lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount", "ratio"})},
      {lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"})},
  });
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count = 2

===============================================
     | amount  ratio  region  actual  predicted
-----------------------------------------------
0    | 1200    0.9    0       0       0        
1    | 1800    1.1    1       0       0        
2    | 3400    0.7    2       0       0        
3    | 4100    1.3    0       1       1        
4    | 5200    0.6    1       1       1        
5    | 6000    1.4    2       1       1        
-----------------------------------------------
[6 cs x 5 feat]
```

## See Also

- [`prep::StandardScaler`](../scaling/StandardScaler.md) — the standalone twin.
- [`options::StandardScaler`](../scaling/StandardScalerOptions.md) — the full
  option reference.
- [`prep::stage::one_hot_encoder`](one_hot_encoder.md) — the categorical stage
  factory.
- [`Model::append`](../../models/Model/append.md),
  [`Model::prepare`](../../models/Model/prepare.md) — where a stage is attached.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
