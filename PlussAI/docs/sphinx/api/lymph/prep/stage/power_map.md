<!-- pluss-hand-authored -->

# `lymph::prep::stage::power_map`

[Back to `prep::stage`](../stage.md)

`power_map` builds an attachable [`prep::Stage`](Stage.md) that applies a fitted power transform (Yeo-Johnson by default) to make a skewed column more symmetric, then standardises it.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::power_map(
    lymph::options::PowerMap options = {});
```

## What It Does

Fitting chooses the power parameter that best symmetrises the routed column; replay applies that power and, when `standardize` is on, a trailing z-score. Unlike [`quantile_map`](quantile_map.md) it is a smooth, monotone, mostly reversible transform — it pulls in a long tail without collapsing the spacing between values. Yeo-Johnson handles zero and negative inputs; Box-Cox needs strictly positive data.

### Option fields

`lymph::prep::PowerMapMode mode = YeoJohnson`
: `YeoJohnson` (any real input) or `BoxCox` (strictly positive input).

`bool standardize = true`
: Apply a z-score after the power step (Example 2).

`std::optional<double> lambda`, `double lambda_min = -5.0`, `double lambda_max = 5.0`, `std::int64_t lambda_grid = 201`
: Fix the power parameter directly, or bound and resolution the search that picks it.

`double variance_floor = 1e-12`
: Guards the standardisation divisor.

## Parameters

`lymph::options::PowerMap options`
: The transform configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::PowerMap`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- `BoxCox` on a column with a zero or negative value is reported at fit time; use `YeoJohnson` there.

## Practical Examples

### Example 1

The default stage, attached to a model. The exponentially growing `income` column is pulled toward symmetry before the estimator sees it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // power_map applies a fitted Yeo-Johnson power transform to make a skewed
  // column more symmetric, then standardises it.
  const pluss::Series income = pluss::make_series({1., 2., 4., 8., 16., 64.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::power_map(), lymph::prep::route::names({"income"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"income", Slate(income)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

================================
     | income  actual  predicted
--------------------------------
0    | 1       0       0        
1    | 2       0       0        
2    | 4       0       0        
3    | 8       1       1        
4    | 16      1       1        
5    | 64      1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

`standardize`, standalone. With it on the output is centred and unit-scaled; with it off you see the raw power-transformed values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // mode picks Yeo-Johnson (handles zero and negative values) or Box-Cox
  // (strictly positive input). standardize toggles the trailing z-score step.
  const pluss::Series v = pluss::make_series({1., 2., 3., 5., 8., 13., 21., 34.}, {8}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::PowerMap yj{};                                       // default: YeoJohnson, standardize=true
  lymph::options::PowerMap yj_raw{};
  yj_raw.standardize = false;

  std::cout << "YeoJohnson + standardize:\n" << lymph::prep::PowerMap{yj}.learn_map(frame) << "\n";
  std::cout << "YeoJohnson, standardize off:\n" << lymph::prep::PowerMap{yj_raw}.learn_map(frame);

  return 0;
}
```

Output

```text
YeoJohnson + standardize:
====================
     | v            
--------------------
0    | -1.523361685 
1    | -1.033884196 
2    | -0.6984357675
3    | -0.2417371219
4    |  0.1968143751
5    |  0.6548842732
6    |  1.103000449 
7    |  1.542719673 
--------------------
[8 cs x 1 feat]
YeoJohnson, standardize off:
===================
     | v           
-------------------
0    | 0.6696700846
1    | 1.040415402 
2    | 1.294494367 
3    | 1.640411979 
4    | 1.972584382 
5    | 2.319540623 
6    | 2.658957609 
7    | 2.992014484 
-------------------
[8 cs x 1 feat]
```

### Example 3

Power-map the skewed numeric column, one-hot the categorical one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1., 3., 9., 27., 81., 243.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::power_map(), lymph::prep::route::names({"income"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"income", Slate(income)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | income  actual  predicted
--------------------------------
0    | 1       0       0        
1    | 3       0       0        
2    | 9       0       0        
3    | 27      1       1        
4    | 81      1       1        
5    | 243     1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::quantile_map`](quantile_map.md) — rank-based, stronger, less faithful.
- [`prep::stage::standard_scaler`](standard_scaler.md) — the standardisation step alone.
- `prep::PowerMap` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
