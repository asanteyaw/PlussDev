<!-- pluss-hand-authored -->

# `lymph::prep::stage::polynomial_lift`

[Back to `prep::stage`](../stage.md)

`polynomial_lift` builds an attachable [`prep::Stage`](Stage.md) that adds squared and interaction terms to the routed columns so a linear model can fit a curved boundary.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::polynomial_lift(
    lymph::options::PolynomialLift options = {});
```

## What It Does

Replay emits the original columns plus every product of them up to `degree`. Two inputs at degree 2 become five columns (`a`, `b`, `a^2`, `a*b`, `b^2`). It **widens the frame** — its [`contract().changes_feature_count`](Stage.md#contract) is `true` — and the width grows fast with `degree` and the number of routed columns, so pair it with a scaler and keep the route small.

### Option fields

`std::int64_t degree = 2`
: Highest total power of the generated terms.

`bool interactions_only = false`
: Keep only cross terms (`a*b`), dropping pure powers (`a^2`) (Example 2).

`bool include_bias = false`
: Prepend a constant `1` column (Example 2).

## Parameters

`lymph::options::PolynomialLift options`
: The transform configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::PolynomialLift`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A large `degree` or a wide route produces a very wide frame; this is a performance concern, not an error.

## Practical Examples

### Example 1

The default (degree 2) stage, attached to a model. The added `a^2` term lets a linear boundary bend around the `y = 1` corners.

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

  // polynomial_lift adds squared and interaction terms so a linear model can fit
  // a curved boundary. Two inputs at degree 2 become five columns.
  const pluss::Series a = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const pluss::Series b = pluss::make_series({1., 0., -1., 0., 1., 2.}, {6}, f64);
  const pluss::Series y = pluss::make_series({1, 0, 0, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Slate(a)}, {"b", Slate(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::polynomial_lift(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"a", Slate(a)}, {"b", Slate(b)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

====================================
     | a     b     actual  predicted
------------------------------------
0    | -2     1    1       1        
1    | -1     0    0       0        
2    |  0    -1    0       0        
3    |  1     0    0       0        
4    |  2     1    1       1        
5    |  3     2    1       1        
------------------------------------
[6 cs x 4 feat]
```

### Example 2

`degree`, `interactions_only`, and `include_bias`, standalone over two columns. `feature_0`/`feature_1` are the originals; the rest are the generated terms.

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

  // degree sets the highest power; interactions_only drops pure powers and keeps
  // only cross terms; include_bias adds a constant column. Standalone view.
  const pluss::Series a = pluss::make_series({1., 2., 3.}, {3}, f64);
  const pluss::Series b = pluss::make_series({4., 5., 6.}, {3}, f64);
  const DataFrame frame({{"a", Slate(a)}, {"b", Slate(b)}});

  lymph::options::PolynomialLift deg2{};                               // default
  lymph::options::PolynomialLift inter{};
  inter.interactions_only = true;
  lymph::options::PolynomialLift bias{};
  bias.include_bias = true;

  std::cout << "degree 2:\n"           << lymph::prep::PolynomialLift{deg2}.learn_map(frame) << "\n";
  std::cout << "interactions_only:\n"  << lymph::prep::PolynomialLift{inter}.learn_map(frame) << "\n";
  std::cout << "include_bias:\n"       << lymph::prep::PolynomialLift{bias}.learn_map(frame);

  return 0;
}
```

Output

```text
degree 2:
============================================================
     | feature_0  feature_1  feature_2  feature_3  feature_4
------------------------------------------------------------
0    | 1          4          1          4          16       
1    | 2          5          4          10         25       
2    | 3          6          9          18         36       
------------------------------------------------------------
[3 cs x 5 feat]
interactions_only:
======================================
     | feature_0  feature_1  feature_2
--------------------------------------
0    | 1          4          4        
1    | 2          5          10       
2    | 3          6          18       
--------------------------------------
[3 cs x 3 feat]
include_bias:
=======================================================================
     | feature_0  feature_1  feature_2  feature_3  feature_4  feature_5
-----------------------------------------------------------------------
0    | 1          1          4          1          4          16       
1    | 1          2          5          4          10         25       
2    | 1          3          6          9          18         36       
-----------------------------------------------------------------------
[3 cs x 6 feat]
```

### Example 3

Lift the numeric pair, scale the lifted result, one-hot the categorical column.

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

  const pluss::Series a = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const pluss::Series b = pluss::make_series({1., 0., -1., 0., 1., 2.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({1,0,0,0,1,1}, {6}, f64);
  const DataFrame x({{"a", Slate(a)}, {"b", Slate(b)}, {"region", Slate(region)}});

  // lift the numeric pair, scale the lifted result, one-hot the categorical one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::polynomial_lift(), lymph::prep::route::names({"a", "b"}))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"a", "b"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"a", Slate(a)}, {"b", Slate(b)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 3

====================================
     | a     b     actual  predicted
------------------------------------
0    | -2     1    1       1        
1    | -1     0    0       0        
2    |  0    -1    0       0        
3    |  1     0    0       0        
4    |  2     1    1       1        
5    |  3     2    1       1        
------------------------------------
[6 cs x 4 feat]
```

## See Also

- [`prep::stage::standard_scaler`](standard_scaler.md) — pair with it; lifted terms have wildly different scales.
- [`prep::stage::custom`](custom.md) — for a single hand-written derived feature.
- `prep::PolynomialLift` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
