<!-- pluss-hand-authored -->

# `lymph::prep::stage::simple_imputer`

[Back to `prep::stage`](../stage_family.md)

`simple_imputer` builds an attachable [`prep::Stage`](Stage.md) that fills the gaps in a routed numeric column with a single fitted statistic — the column mean by default.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::simple_imputer(
    lymph::options::SimpleImputer options = {});
```

## What It Does

Fitting computes one fill value per routed column from the observed (non-missing) entries; replay substitutes it for every gap. It is the cheapest gap filler and the right default when missingness is incidental. When the missing pattern itself carries signal, or the column is strongly predictive, a model-based filler ([`neighbor_imputer`](neighbor_imputer.md), [`iterative_imputer`](iterative_imputer.md)) or an explicit missing-indicator is usually better.

### Option fields

`lymph::prep::SimpleImputerStrategy strategy = Mean`
: `Mean`, `Median`, `MostFrequent`, or `Constant` (Example 2).

`double constant_value = 0.0`
: The fill used when `strategy == Constant`.

`double empty_feature_fallback = 0.0`
: The fill used for a column that had no observed value at all at fit time.

## Parameters

`lymph::options::SimpleImputer options`
: The imputer configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::SimpleImputer`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A non-numeric routed column is reported when the owning plan fits the stage.

## Practical Examples

### Example 1

The default (mean) stage, attached to a model. The two gaps are filled with the mean of the observed values before the estimator trains.

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

  // A numeric column with two gaps. The default stage fills them with the mean
  // of the observed values (here 32.5), then the estimator trains on the
  // completed column.
  const pluss::Series amount = pluss::make_series({10., 20., NaN, 40., NaN, 60.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::simple_imputer(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Thread(amount)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

================================
     | amount  actual  predicted
--------------------------------
0    | 10      0       0        
1    | 20      0       0        
2    | NULL    1       1        
3    | 40      1       1        
4    | NULL    1       1        
5    | 60      1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

`strategy` selects the fill statistic, shown standalone over one gappy column with an outlier. Mean is dragged by the outlier; median and a fixed constant are not.

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

  // strategy selects the fill statistic. Standalone view over one gappy column:
  // mean, median, and a fixed constant.
  const pluss::Series v = pluss::make_series({1., 2., 100., NaN, 4., NaN}, {6}, f64);
  const DataFrame frame({{"v", Thread(v)}});

  lymph::options::SimpleImputer mean_opt{};                                  // default: Mean
  lymph::options::SimpleImputer median_opt{};
  median_opt.strategy = lymph::prep::SimpleImputerStrategy::Median;
  lymph::options::SimpleImputer const_opt{};
  const_opt.strategy = lymph::prep::SimpleImputerStrategy::Constant;
  const_opt.constant_value = -1.0;

  std::cout << "Mean:\n"     << lymph::prep::SimpleImputer{mean_opt}.learn_map(frame) << "\n";
  std::cout << "Median:\n"   << lymph::prep::SimpleImputer{median_opt}.learn_map(frame) << "\n";
  std::cout << "Constant -1:\n" << lymph::prep::SimpleImputer{const_opt}.learn_map(frame);

  return 0;
}
```

Output

```text
Mean:
============
     | v    
------------
0    | 1    
1    | 2    
2    | 100  
3    | 26.75
4    | 4    
5    | 26.75
------------
[6 cs x 1 feat]
Median:
===========
     | v   
-----------
0    | 1   
1    | 2   
2    | 100 
3    | 3   
4    | 4   
5    | 3   
-----------
[6 cs x 1 feat]
Constant -1:
===========
     | v   
-----------
0    |  1  
1    |  2  
2    |  100
3    | -1  
4    |  4  
5    | -1  
-----------
[6 cs x 1 feat]
```

### Example 3

Impute then scale the numeric column, one-hot the categorical one — three chained stages.

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

  const pluss::Series a = pluss::make_series({10., 20., NaN, 40., 50., 60.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"region", Thread(region)}});

  // impute then scale the numeric column, one-hot the categorical one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::simple_imputer(), lymph::prep::route::names({"a"}))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"a"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"a", Thread(a)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 3

==============================
     | a     actual  predicted
------------------------------
0    | 10    0       0        
1    | 20    0       0        
2    | NULL  0       0        
3    | 40    1       1        
4    | 50    1       1        
5    | 60    1       1        
------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::neighbor_imputer`](neighbor_imputer.md), [`prep::stage::iterative_imputer`](iterative_imputer.md) — model-based fills.
- `prep::SimpleImputer` — the standalone twin.
- [`Model::append`](../../models/Model/append.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
