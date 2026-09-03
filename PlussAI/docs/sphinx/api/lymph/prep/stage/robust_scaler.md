<!-- pluss-hand-authored -->

# `lymph::prep::stage::robust_scaler`

[Back to `prep::stage`](../stage_family.md)

`robust_scaler` builds an attachable [`prep::Stage`](Stage.md) that centres each
routed numeric column on its **median** and scales by its **interquartile
range** — a scaling that a handful of extreme values barely disturbs. It is the
stage-shaped counterpart of the standalone `prep::RobustScaler`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::robust_scaler(
    lymph::options::RobustScaler options = {});
```

`lymph::options::RobustScaler` is the canonical spelling of the option struct;
`lymph::prep::RobustScalerOptions` is the family-scoped alias for the same type.

## What It Does

The call returns a `Stage` descriptor. Once fitted, replay maps a value `x` to
`(x - median) / (q_high - q_low)`, where `q_low` / `q_high` are the configured
lower and upper quantiles (25th and 75th percentile by default).

The point of the median and the quartile range — rather than the mean and the
standard deviation that [`standard_scaler`](standard_scaler.md) uses — is
resistance to outliers. A single very large value pulls the mean and inflates the
standard deviation, which squashes every other row toward zero. The median and
the quartiles are almost unmoved by it, so the bulk of the column keeps a useful
spread (Example 2).

### Option fields

`bool with_centering = true`
: Subtract the column median. Turn off to scale by spread while leaving the
  column's location in place (Example 3).

`bool with_scaling = true`
: Divide by the quantile range. Turn off to centre on the median without
  changing the unit of measure.

`double lower_quantile = 0.25`, `double upper_quantile = 0.75`
: The percentiles that define the spread window. The default is the
  interquartile range; a wider band such as `0.10` / `0.90` yields a larger
  divisor and therefore smaller scaled values (Example 3).

`double range_floor = 1e-12`
: Lower bound on `q_high - q_low`, guarding the division for a near-constant
  column.

The full field reference lives on the
[`options::RobustScaler` page](../scaling/RobustScalerOptions.md).

## Parameters

`lymph::options::RobustScaler options`
: The scaler configuration. Defaulted, so `robust_scaler()` is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::RobustScaler` and the given
options. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- `lower_quantile >= upper_quantile` collapses the spread window; keep them
  ordered and inside `(0, 1)`.
- A route that resolves to a non-numeric column is reported when the owning plan
  fits the stage, not here.

## Practical Examples

### Example 1

The default stage, attached to a model.

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
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage centres each routed column on its median and scales by
  // its interquartile range (Q1..Q3). No options argument means a
  // default-constructed lymph::options::RobustScaler.
  model.append(lymph::prep::stage::robust_scaler(),
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

Why "robust". Five clustered values plus one extreme outlier. `robust_scaler`
leaves the first five rows essentially where they were; `standard_scaler`, driven
by the mean and standard deviation, squashes them toward zero.

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

  // Five clustered values and one extreme outlier (1000).
  const pluss::Series clean = pluss::make_series({10.0, 12.0, 14.0, 16.0, 18.0, 20.0}, {6}, f64);
  const pluss::Series spiked = pluss::make_series({10.0, 12.0, 14.0, 16.0, 18.0, 1000.0}, {6}, f64);
  const DataFrame clean_df({{"v", Thread(clean)}});
  const DataFrame spiked_df({{"v", Thread(spiked)}});

  // robust_scaler uses the median and IQR, so the quartile-based centre and
  // scale barely move when one value blows up. standard_scaler, driven by the
  // mean and standard deviation, is dragged toward the outlier.
  std::cout << "robust_scaler, no outlier:\n";
  std::cout << lymph::prep::RobustScaler{}.learn_map(clean_df) << "\n";
  std::cout << "robust_scaler, with outlier (first five rows still ~unchanged):\n";
  std::cout << lymph::prep::RobustScaler{}.learn_map(spiked_df) << "\n";
  std::cout << "standard_scaler, same outlier (first five rows squashed):\n";
  std::cout << lymph::prep::StandardScaler{}.learn_map(spiked_df);

  return 0;
}
```

Output

```text
robust_scaler, no outlier:
===========
     | v   
-----------
0    | -1  
1    | -0.6
2    | -0.2
3    |  0.2
4    |  0.6
5    |  1  
-----------
[6 cs x 1 feat]
robust_scaler, with outlier (first five rows still ~unchanged):
===========
     | v   
-----------
0    | -1  
1    | -0.6
2    | -0.2
3    |  0.2
4    |  0.6
5    |  197
-----------
[6 cs x 1 feat]
standard_scaler, same outlier (first five rows squashed):
====================
     | v            
--------------------
0    | -0.4580878107
1    | -0.4526451833
2    | -0.4472025558
3    | -0.4417599284
4    | -0.4363173009
5    |  2.236012779 
--------------------
[6 cs x 1 feat]
```

### Example 3

`with_scaling` off, and a wider quantile band. Family-alias spelling of the
option struct.

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

  const pluss::Series v = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64);
  const DataFrame frame({{"v", Thread(v)}});

  // with_centering / with_scaling switch the two steps independently, exactly
  // like standard_scaler. lower_quantile / upper_quantile widen or narrow the
  // spread window: the default 0.25/0.75 is the IQR; 0.10/0.90 uses a wider
  // band. Family-alias spelling for the option struct.
  lymph::prep::RobustScalerOptions centre_only{};
  centre_only.with_scaling = false;

  lymph::prep::RobustScalerOptions wide_band{};
  wide_band.lower_quantile = 0.10;
  wide_band.upper_quantile = 0.90;

  std::cout << "with_scaling = false (median removed, spread untouched):\n";
  std::cout << lymph::prep::RobustScaler{centre_only}.learn_map(frame) << "\n";
  std::cout << "quantiles 0.10 / 0.90 (wider band -> smaller scaled values):\n";
  std::cout << lymph::prep::RobustScaler{wide_band}.learn_map(frame);

  return 0;
}
```

Output

```text
with_scaling = false (median removed, spread untouched):
===========
     | v   
-----------
0    | -4.5
1    | -3.5
2    | -2.5
3    | -1.5
4    | -0.5
5    |  0.5
6    |  1.5
7    |  2.5
8    |  3.5
9    |  4.5
-----------
[10 cs x 1 feat]
quantiles 0.10 / 0.90 (wider band -> smaller scaled values):
=====================
     | v             
---------------------
0    | -0.625        
1    | -0.4861111111 
2    | -0.3472222222 
3    | -0.2083333333 
4    | -0.06944444444
5    |  0.06944444444
6    |  0.2083333333 
7    |  0.3472222222 
8    |  0.4861111111 
9    |  0.625        
---------------------
[10 cs x 1 feat]
```

### Example 4

Robust scaling on the numeric column, one-hot on the categorical one, chained.
Fully qualified factory spelling.

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

  // Robust scaling on the numeric column, one-hot on the categorical one,
  // chained. Fully qualified factory spelling.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::robust_scaler(),
               lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
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

- `prep::RobustScaler` — the standalone twin (`prep::scaling`).
- [`options::RobustScaler`](../scaling/RobustScalerOptions.md) — the full option
  reference.
- [`prep::stage::standard_scaler`](standard_scaler.md) — mean / standard-deviation
  scaling; simpler, but outlier-sensitive.
- [`Model::append`](../../models/Model/append.md),
  [`Model::prepare`](../../models/Model/prepare.md) — where a stage is attached.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
