<!-- pluss-hand-authored -->

# `lymph::prep::stage::quantile_map`

[Back to `prep::stage`](../stage.md)

`quantile_map` builds an attachable [`prep::Stage`](Stage.md) that re-maps a routed column onto its own rank distribution — a uniform spread by default, or a Gaussian shape. Skew, heavy tails, and single extreme values are flattened away.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::quantile_map(
    lymph::options::QuantileMap options = {});
```

## What It Does

Fitting records the sorted values of the routed column; replay converts each value to its quantile (its position in that distribution) and then, for `Gaussian` output, through the inverse normal CDF. Because it works on ranks, the exact magnitude of an outlier stops mattering — only its order does. That makes it a strong pre-step for models sensitive to scale and tails, at the cost of a non-linear, non-invertible remap that can blur genuinely informative spacing between values.

### Option fields

`lymph::prep::QuantileMapOutput output = Uniform`
: `Uniform` spreads values evenly on `[0, 1]`; `Gaussian` maps them onto a standard-normal shape (Example 2).

`double clip = 1e-7`
: Keeps the `Gaussian` mapping away from `+/- infinity` at the extreme quantiles.

## Parameters

`lymph::options::QuantileMap options`
: The transform configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::QuantileMap`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A near-constant column has almost no spread to map; the result is close to a single value.

## Practical Examples

### Example 1

The default (uniform) stage, attached to a model. The `500` outlier becomes just "the largest rank" and stops dominating.

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

  // quantile_map re-maps a column onto its own rank distribution -- uniform by
  // default. Skew and outliers are flattened out.
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 500.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::quantile_map(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Slate(amount)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

================================
     | amount  actual  predicted
--------------------------------
0    | 1       0       0        
1    | 2       0       0        
2    | 3       0       0        
3    | 4       1       1        
4    | 5       1       1        
5    | 500     1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

`output`, standalone over an exponentially spaced column. `Uniform` gives evenly spaced values on `[0, 1]`; `Gaussian` gives a symmetric bell-shaped spread.

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

  // output = Uniform spreads values evenly on [0, 1]; output = Gaussian maps
  // them onto a standard normal shape. Standalone over a skewed column.
  const pluss::Series v = pluss::make_series({1., 2., 4., 8., 16., 32., 64., 128.}, {8}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::QuantileMap uni{};                                   // default: Uniform
  lymph::options::QuantileMap gauss{};
  gauss.output = lymph::prep::QuantileMapOutput::Gaussian;

  std::cout << "Uniform:\n"  << lymph::prep::QuantileMap{uni}.learn_map(frame) << "\n";
  std::cout << "Gaussian:\n" << lymph::prep::QuantileMap{gauss}.learn_map(frame);

  return 0;
}
```

Output

```text
Uniform:
=============
     | v     
-------------
0    | 0.0625
1    | 0.1875
2    | 0.3125
3    | 0.4375
4    | 0.5625
5    | 0.6875
6    | 0.8125
7    | 0.9375
-------------
[8 cs x 1 feat]
Gaussian:
====================
     | v            
--------------------
0    | -1.534120545 
1    | -0.8871465585
2    | -0.4887764115
3    | -0.1573106846
4    |  0.1573106846
5    |  0.4887764115
6    |  0.8871465585
7    |  1.534120545 
--------------------
[8 cs x 1 feat]
```

### Example 3

Quantile-map the skewed numeric column, one-hot the categorical one.

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

  const pluss::Series amount = pluss::make_series({10., 12., 15., 900., 20., 25.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::quantile_map(), lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"amount", Slate(amount)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | amount  actual  predicted
--------------------------------
0    | 10      0       0        
1    | 12      0       0        
2    | 15      0       0        
3    | 900     1       1        
4    | 20      1       1        
5    | 25      1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::power_map`](power_map.md) — a smooth parametric alternative for skew.
- [`prep::stage::robust_scaler`](robust_scaler.md) — keeps the shape, just resists outliers.
- `prep::QuantileMap` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
