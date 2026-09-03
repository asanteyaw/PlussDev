<!-- pluss-hand-authored -->

# `lymph::prep::stage::iterative_imputer`

[Back to `prep::stage`](../stage_family.md)

`iterative_imputer` builds an attachable [`prep::Stage`](Stage.md) that fills gaps by modelling each column with missing values as a ridge regression on the others, repeated for a fixed number of rounds.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::iterative_imputer(
    lymph::options::IterativeImputer options = {});
```

## What It Does

Fitting starts from a simple fill, then round after round refits each incomplete column against the current estimates of the others and updates its gaps. It captures multi-column structure that [`neighbor_imputer`](neighbor_imputer.md) approximates locally, and converges quickly when the relationship is close to linear. Route it at the whole block of numeric columns you want it to reason over.

### Option fields

`std::int64_t rounds = 8`
: Number of refinement passes. More rounds help when the columns are mutually entangled; on a clean linear relationship it converges in one (Example 2).

`double ridge = 1e-6`
: Regularisation strength of the per-column regression.

`double empty_feature_fallback = 0.0`
: Fill for a column that cannot be modelled.

## Parameters

`lymph::options::IterativeImputer options`
: The imputer configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::IterativeImputer`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A single routed column leaves the regression with no predictors; route the block together.

## Practical Examples

### Example 1

The default stage, attached to a model. `b` is roughly `2*a`, so the gaps in `b` are recovered from `a`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // iterative_imputer models each column with gaps as a ridge regression on the
  // others and repeats for a fixed number of rounds.
  const pluss::Series a = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.}, {8}, f64);
  const pluss::Series b = pluss::make_series({2., 4., NaN, 8., 10., 12., NaN, 16.}, {8}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1}, {8}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::iterative_imputer(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"b", Thread(b)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

==============================
     | b     actual  predicted
------------------------------
0    | 2     0       0        
1    | 4     0       0        
2    | NULL  0       0        
3    | 8     0       0        
4    | 10    1       1        
5    | 12    1       1        
6    | NULL  1       1        
7    | 16    1       1        
------------------------------
[8 cs x 3 feat]
```

### Example 2

`rounds`, standalone. `b` is exactly `2*a`, so the fill for row 2 recovers ~6 after a single round; more rounds change nothing here. Extra rounds matter when columns depend on each other in a tangle.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // rounds controls how many refinement passes run; ridge is the regression
  // regulariser. b is exactly 2*a, so the fill should recover 6 for row 2.
  const pluss::Series a = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series b = pluss::make_series({2., 4., NaN, 8., 10., 12.}, {6}, f64);
  const DataFrame frame({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::options::IterativeImputer few{};
  few.rounds = 1;
  lymph::options::IterativeImputer many{};
  many.rounds = 20;

  std::cout << "rounds = 1:\n"  << lymph::prep::IterativeImputer{few}.learn_map(frame) << "\n";
  std::cout << "rounds = 20:\n" << lymph::prep::IterativeImputer{many}.learn_map(frame);

  return 0;
}
```

Output

```text
rounds = 1:
=======================
     | a     b         
-----------------------
0    | 1     2         
1    | 2     4         
2    | 3     6.00000007
3    | 4     8         
4    | 5     10        
5    | 6     12        
-----------------------
[6 cs x 2 feat]
rounds = 20:
=======================
     | a     b         
-----------------------
0    | 1     2         
1    | 2     4         
2    | 3     6.00000007
3    | 4     8         
4    | 5     10        
5    | 6     12        
-----------------------
[6 cs x 2 feat]
```

### Example 3

Impute the numeric block, minmax-scale it, one-hot the categorical column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  const pluss::Series x1 = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series x2 = pluss::make_series({3.,6.,NaN,12.,15.,18.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::iterative_imputer(), lymph::prep::route::names({"x1", "x2"}))
       .append(lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"x1", "x2"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"x2", Thread(x2)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 3

==============================
     | x2    actual  predicted
------------------------------
0    | 3     0       0        
1    | 6     0       0        
2    | NULL  0       0        
3    | 12    1       1        
4    | 15    1       1        
5    | 18    1       1        
------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::simple_imputer`](simple_imputer.md), [`prep::stage::neighbor_imputer`](neighbor_imputer.md).
- `prep::IterativeImputer` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
