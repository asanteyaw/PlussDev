<!-- pluss-hand-authored -->

# `lymph::prep::stage::neighbor_imputer`

[Back to `prep::stage`](../stage_family.md)

`neighbor_imputer` builds an attachable [`prep::Stage`](Stage.md) that fills each gap from the most similar complete rows, using the other routed columns to measure similarity.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::neighbor_imputer(
    lymph::options::NeighborImputer options = {});
```

## What It Does

Fitting keeps the training rows; replay finds, for each row with a gap, its `neighbors` nearest complete rows and averages their value for the missing column. It respects relationships between columns that [`simple_imputer`](simple_imputer.md) ignores, at the cost of holding the training data and a nearest-neighbour search at replay time. Route it at the whole group of related columns, not one in isolation.

### Option fields

`std::int64_t neighbors = 5`
: How many nearest complete rows are averaged (Example 2).

`lymph::prep::NeighborImputerWeighting weighting = Distance`
: `Uniform` averages the neighbours equally; `Distance` weights closer rows more (Example 2).

`double distance_floor = 1e-12`
: Guards the distance weighting against a zero-distance duplicate row.

`double empty_feature_fallback = 0.0`
: Fill for a column with no usable neighbours.

## Parameters

`lymph::options::NeighborImputer options`
: The imputer configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::NeighborImputer`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A route of a single column gives the search nothing to work with; route the related columns together.

## Practical Examples

### Example 1

The default stage, attached to a model. `w` tracks `h`, so the gaps in `w` are filled from rows with a similar `h`.

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

  // neighbor_imputer fills a gap from the most similar complete rows, using the
  // other columns to measure similarity.
  const pluss::Series h = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series w = pluss::make_series({1.1, 2.0, NaN, 4.1, 5.0, NaN}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"h", Thread(h)}, {"w", Thread(w)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::neighbor_imputer(), lymph::prep::route::names({"h", "w"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"h", Thread(h)}, {"w", Thread(w)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

====================================
     | h     w     actual  predicted
------------------------------------
0    | 1     1.1   0       0        
1    | 2     2     0       0        
2    | 3     NULL  0       0        
3    | 4     4.1   1       1        
4    | 5     5     1       1        
5    | 6     NULL  1       1        
------------------------------------
[6 cs x 4 feat]
```

### Example 2

`neighbors` and `weighting`, shown standalone. `b` tracks `a` with scatter, so `k = 2` uniform and `k = 4` distance-weighted fill the gap at row 2 differently.

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

  // neighbors sets how many similar rows are averaged; weighting chooses uniform
  // or distance-weighted averaging. Here column b tracks a with some scatter, so
  // the fill depends on which rows count as "near".
  const pluss::Series a = pluss::make_series({1., 2., 3., 4., 10., 11., 12., 13.}, {8}, f64);
  const pluss::Series b = pluss::make_series({2., 5., NaN, 9., 21., 24., 23., 27.}, {8}, f64);
  const DataFrame frame({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::options::NeighborImputer k2{};
  k2.neighbors = 2;
  lymph::options::NeighborImputer k4_dist{};
  k4_dist.neighbors = 4;
  k4_dist.weighting = lymph::prep::NeighborImputerWeighting::Distance;

  std::cout << "neighbors=2, uniform:\n" << lymph::prep::NeighborImputer{k2}.learn_map(frame) << "\n";
  std::cout << "neighbors=4, distance-weighted:\n" << lymph::prep::NeighborImputer{k4_dist}.learn_map(frame);

  return 0;
}
```

Output

```text
neighbors=2, uniform:
=================
     | a     b   
-----------------
0    | 1     2   
1    | 2     5   
2    | 3     7   
3    | 4     9   
4    | 10    21  
5    | 11    24  
6    | 12    23  
7    | 13    27  
-----------------
[8 cs x 2 feat]
neighbors=4, distance-weighted:
========================
     | a     b          
------------------------
0    | 1     2          
1    | 2     5          
2    | 3     6.810810811
3    | 4     9          
4    | 10    21         
5    | 11    24         
6    | 12    23         
7    | 13    27         
------------------------
[8 cs x 2 feat]
```

### Example 3

Impute the related numeric columns together, scale them, one-hot the categorical one.

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
  const pluss::Series x2 = pluss::make_series({2.,4.,NaN,8.,10.,12.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::neighbor_imputer(), lymph::prep::route::names({"x1", "x2"}))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"x1", "x2"}))
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
0    | 2     0       0        
1    | 4     0       0        
2    | NULL  0       0        
3    | 8     1       1        
4    | 10    1       1        
5    | 12    1       1        
------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::simple_imputer`](simple_imputer.md) — one constant per column.
- [`prep::stage::iterative_imputer`](iterative_imputer.md) — regression-based, iterated.
- `prep::NeighborImputer` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
