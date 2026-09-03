<!-- pluss-hand-authored -->

# `lymph::prep::stage::vector_normalizer`

[Back to `prep::stage`](../stage_family.md)

`vector_normalizer` builds an attachable [`prep::Stage`](Stage.md) that scales each **row** of the routed columns to unit norm, so only the direction of the feature vector matters and its overall length is discarded.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::vector_normalizer(
    lymph::options::VectorNormalizer options = {});
```

## What It Does

It is a row-wise operation, unlike every scaler on this list — it divides each row by its own norm rather than adjusting a column against its statistics, so it keeps no fitted state. Use it when the pattern across a group of features matters but the total magnitude does not (counts that should be read as proportions, embeddings compared by angle). Two rows pointing the same way become identical, which is the point and also the risk.

### Option fields

`lymph::prep::VectorNorm norm = L2`
: `L1` (magnitudes sum to 1), `L2` (Euclidean unit length), or `Linf` (largest magnitude becomes 1) (Example 2).

`double floor = 1e-12`
: Guards the division for an all-zero row.

## Parameters

`lymph::options::VectorNormalizer options`
: The transform configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::VectorNormalizer`, with `contract().stateful == false`. Cheap to copy.

## Errors

- The factory itself does not throw.
- Routing a single column makes every non-zero row normalise to `+/- 1`; route the group together.

## Practical Examples

### Example 1

The default (L2) stage, attached to a model. Each `(a, b)` row is scaled to unit length before the estimator sees it.

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

  // vector_normalizer scales each ROW of the routed columns to unit norm (L2 by
  // default), so only the direction of the feature vector matters -- its length
  // is discarded. Rows that point the same way become identical.
  const pluss::Series a = pluss::make_series({3., 1., 0., 6., 8., 2.}, {6}, f64);
  const pluss::Series b = pluss::make_series({4., 3., 5., 8., 1., 7.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 0, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::vector_normalizer(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"a", Thread(a)}, {"b", Thread(b)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

====================================
     | a     b     actual  predicted
------------------------------------
0    | 3     4     0       0        
1    | 1     3     0       0        
2    | 0     5     1       1        
3    | 6     8     0       0        
4    | 8     1     0       0        
5    | 2     7     1       1        
------------------------------------
[6 cs x 4 feat]
```

### Example 2

`norm`, standalone over a two-column frame. L1 rows sum to 1 in magnitude; L2 rows have Euclidean length 1; Linf rows have a largest entry of 1.

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

  // norm selects L1 (values sum to 1 in magnitude), L2 (Euclidean unit length),
  // or Linf (largest magnitude becomes 1). Standalone over a two-column frame.
  const pluss::Series a = pluss::make_series({3., 1., 4., 0.}, {4}, f64);
  const pluss::Series b = pluss::make_series({4., 1., 0., 5.}, {4}, f64);
  const DataFrame frame({{"a", Thread(a)}, {"b", Thread(b)}});

  for (auto [label, n] : {std::pair{"L1", lymph::prep::VectorNorm::L1},
                          std::pair{"L2", lymph::prep::VectorNorm::L2},
                          std::pair{"Linf", lymph::prep::VectorNorm::Linf}}) {
    lymph::options::VectorNormalizer o{};
    o.norm = n;
    std::cout << label << ":\n" << lymph::prep::VectorNormalizer{o}.learn_map(frame) << "\n";
  }

  return 0;
}
```

Output

```text
L1:
=================================
     | a             b           
---------------------------------
0    | 0.4285714286  0.5714285714
1    | 0.5           0.5         
2    | 1             0           
3    | 0             1           
---------------------------------
[4 cs x 2 feat]
L2:
=================================
     | a             b           
---------------------------------
0    | 0.6           0.8         
1    | 0.7071067812  0.7071067812
2    | 1             0           
3    | 0             1           
---------------------------------
[4 cs x 2 feat]
Linf:
=================
     | a     b   
-----------------
0    | 0.75  1   
1    | 1     1   
2    | 1     0   
3    | 0     1   
-----------------
[4 cs x 2 feat]
```

### Example 3

Normalise the numeric pair, one-hot the categorical column.

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

  const pluss::Series a = pluss::make_series({3., 1., 0., 6., 8., 2.}, {6}, f64);
  const pluss::Series b = pluss::make_series({4., 3., 5., 8., 1., 7.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 0, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::vector_normalizer(), lymph::prep::route::names({"a", "b"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | region  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       0       0        
2    | 2       1       1        
3    | 0       0       0        
4    | 1       0       0        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::standard_scaler`](standard_scaler.md), [`prep::stage::minmax_scaler`](minmax_scaler.md) — column-wise scalers.
- `prep::VectorNormalizer` — the standalone twin.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
