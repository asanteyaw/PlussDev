<!-- pluss-hand-authored -->

# `lymph::prep::stage::minmax_scaler`

[Back to `prep::stage`](../stage_family.md)

`minmax_scaler` builds an attachable [`prep::Stage`](Stage.md) that rescales each
routed numeric column onto a fixed range — by default `[0, 1]`. It is the
stage-shaped counterpart of the standalone `prep::MinMaxScaler`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::minmax_scaler(
    lymph::options::MinMaxScaler options = {});
```

`lymph::options::MinMaxScaler` is the canonical spelling of the option struct;
`lymph::prep::MinMaxScalerOptions` is the family-scoped alias for the same type.

## What It Does

The call returns a `Stage` descriptor — transform kind plus options, no fitted
state. Once paired with a [`prep::Route`](../route/Route.md) and attached to a
plan, fitting records each routed column's observed minimum and maximum. Replay
maps a value `x` to `lower + (x - min) / (max - min) * (upper - lower)`, so the
learned minimum lands on `lower`, the learned maximum on `upper`, and everything
in between is spread linearly.

Unlike [`standard_scaler`](standard_scaler.md), the result is not centred on
zero and its spread is not unit variance — it is a bounded range. Reach for it
when a downstream step expects inputs in a known interval, or when preserving the
*shape* of the distribution (no re-centring) matters.

### Option fields

`double lower = 0.0`, `double upper = 1.0`
: The target range. The learned column minimum maps to `lower`, the maximum to
  `upper` (Example 2).

`bool clip = false`
: When `true`, values outside the learned `[min, max]` at replay time are held
  to the target range instead of being allowed past `lower` / `upper`
  (Example 3). Relevant only when later frames can exceed the fitted range.

`double range_floor = 1e-12`
: Lower bound on `max - min`. A near-constant column has almost no range; the
  floor stops the division from exploding. Rarely changed.

The full field reference lives on the
[`options::MinMaxScaler` page](../scaling/MinMaxScalerOptions.md).

## Parameters

`lymph::options::MinMaxScaler options`
: The scaler configuration. Defaulted, so `minmax_scaler()` is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::MinMaxScaler` and the given
options. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- `lower == upper` collapses the target range; keep `lower < upper`.
- A route that resolves to a non-numeric column is reported when the owning plan
  fits the stage, not here.

## Practical Examples

### Example 1

The default stage, attached to a model. Two features on unrelated scales are
each rescaled to `[0, 1]` before the estimator sees them.

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

  // Two features on unrelated scales: an amount in the thousands, a ratio near 1.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage rescales each routed column into [0, 1]. No options
  // argument means a default-constructed lymph::options::MinMaxScaler.
  model.append(lymph::prep::stage::minmax_scaler(),
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

`lower` / `upper`. The default range is `[0, 1]`; a second scaler targets
`[-1, 1]`. The stage factory and the standalone object take the identical option
struct.

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

  // lower / upper set the target range. The default is [0, 1]; here a second
  // scaler targets [-1, 1]. The stage factory and the standalone object take
  // the identical option struct.
  lymph::options::MinMaxScaler unit{};                       // [0, 1]
  lymph::options::MinMaxScaler signed_range{.lower = -1.0, .upper = 1.0};

  const lymph::prep::Stage staged = lymph::prep::stage::minmax_scaler(signed_range);
  std::cout << "staged is a MinMaxScaler stage: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::MinMaxScaler) << "\n\n";

  std::cout << "default range [0, 1]:\n";
  std::cout << lymph::prep::MinMaxScaler{unit}.learn_map(frame) << "\n";
  std::cout << "range [-1, 1]:\n";
  std::cout << lymph::prep::MinMaxScaler{signed_range}.learn_map(frame);

  return 0;
}
```

Output

```text
staged is a MinMaxScaler stage: true

default range [0, 1]:
============
     | price
------------
0    | 0    
1    | 0.25 
2    | 0.5  
3    | 0.75 
4    | 1    
------------
[5 cs x 1 feat]
range [-1, 1]:
============
     | price
------------
0    | -1   
1    | -0.5 
2    |  0   
3    |  0.5 
4    |  1   
------------
[5 cs x 1 feat]
```

### Example 3

`clip`. The scaler is fitted on `10..50`, then applied to a frame whose values
run past both ends of that learned range. With `clip = false` (default) the
results go outside `[0, 1]`; with `clip = true` they are held to it.

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

  // Fit the scaler on 10..50, then map a frame whose values run past both ends
  // of the learned range.
  const pluss::Series train = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const pluss::Series later = pluss::make_series({0.0, 25.0, 50.0, 75.0}, {4}, f64);
  const DataFrame train_df({{"v", Thread(train)}});
  const DataFrame later_df({{"v", Thread(later)}});

  // clip = false (default): out-of-range inputs map outside [0, 1].
  lymph::prep::MinMaxScaler open{lymph::options::MinMaxScaler{}};
  open.learn(train_df);

  // clip = true: results are held inside [0, 1].
  lymph::options::MinMaxScaler clipped_opts{};
  clipped_opts.clip = true;
  lymph::prep::MinMaxScaler clipped{clipped_opts};
  clipped.learn(train_df);

  std::cout << "clip = false (0 -> -0.25, 75 -> 1.625):\n";
  std::cout << open.map(later_df) << "\n";
  std::cout << "clip = true (held to [0, 1]):\n";
  std::cout << clipped.map(later_df);

  return 0;
}
```

Output

```text
clip = false (0 -> -0.25, 75 -> 1.625):
=============
     | v     
-------------
0    | -0.25 
1    |  0.375
2    |  1    
3    |  1.625
-------------
[4 cs x 1 feat]
clip = true (held to [0, 1]):
============
     | v    
------------
0    | 0    
1    | 0.375
2    | 1    
3    | 1    
------------
[4 cs x 1 feat]
```

### Example 4

One step among several: minmax on the numeric column, one-hot on the categorical
one, passed as a single batch to `append`. Fully qualified factory spelling.

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
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"region", Thread(region)}});

  // One step among several: minmax on the numeric column, one-hot on the
  // categorical one, passed as a single batch to append. Fully qualified
  // factory spelling.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append({
      {lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"amount"})},
      {lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"})},
  });
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  const DataFrame review({
      {"amount", Thread(amount)},
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

========================================
     | amount  region  actual  predicted
----------------------------------------
0    | 1200    0       0       0        
1    | 1800    1       0       0        
2    | 3400    2       0       0        
3    | 4100    0       1       1        
4    | 5200    1       1       1        
5    | 6000    2       1       1        
----------------------------------------
[6 cs x 4 feat]
```

## See Also

- `prep::MinMaxScaler` — the standalone twin (`prep::scaling`).
- [`options::MinMaxScaler`](../scaling/MinMaxScalerOptions.md) — the full option
  reference.
- [`prep::stage::standard_scaler`](standard_scaler.md) — zero-mean / unit-variance
  scaling instead of a bounded range.
- [`prep::stage::maxabs_scaler`](maxabs_scaler.md) — divide by maximum magnitude,
  no shift.
- [`Model::append`](../../models/Model/append.md),
  [`Model::prepare`](../../models/Model/prepare.md) — where a stage is attached.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
