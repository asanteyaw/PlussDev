<!-- pluss-hand-authored -->

# `lymph::prep::stage::maxabs_scaler`

[Back to `prep::stage`](../stage.md)

`maxabs_scaler` builds an attachable [`prep::Stage`](Stage.md) that divides each
routed numeric column by its largest absolute value, mapping it into `[-1, 1]`
**without shifting zero**. It is the stage-shaped counterpart of the standalone
`prep::MaxAbsScaler`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::maxabs_scaler(
    lymph::options::MaxAbsScaler options = {});
```

`lymph::options::MaxAbsScaler` is the canonical spelling of the option struct;
`lymph::prep::MaxAbsScalerOptions` is the family-scoped alias for the same type.

## What It Does

The call returns a `Stage` descriptor. Once fitted, replay maps a value `x` to
`x / max(|column|)`. There is no subtraction, so a zero in the input stays a zero
in the output, positive stays positive, and negative stays negative — only the
magnitude changes. Every fitted value lands in `[-1, 1]`; a later value larger in
magnitude than anything seen at fit time maps outside it.

This is the scaler to reach for when the sign and the "zeroness" of a feature
carry meaning — signed deltas, centred measurements, or already-sparse columns
where subtracting a mean would destroy the sparsity. When you instead want a
bounded range with an arbitrary floor and ceiling, use
[`minmax_scaler`](minmax_scaler.md); when you want zero mean and unit variance,
use [`standard_scaler`](standard_scaler.md).

### Option fields

`double floor = 1e-12`
: Lower bound on `max(|column|)`. An all-zero (or near-zero) column has no
  magnitude to divide by; the floor keeps the division finite. This is the only
  field.

The full field reference lives on the
[`options::MaxAbsScaler` page](../scaling/MaxAbsScalerOptions.md).

## Parameters

`lymph::options::MaxAbsScaler options`
: The scaler configuration. Defaulted, so `maxabs_scaler()` is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::MaxAbsScaler` and the given
options. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
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
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage divides each routed column by its largest absolute
  // value, mapping it into [-1, 1] without shifting zero. No options argument
  // means a default-constructed lymph::options::MaxAbsScaler.
  model.append(lymph::prep::stage::maxabs_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
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

Sign and zero are preserved. A column with both signs and a zero; the largest
magnitude is `8`, so every value is divided by `8`.

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

  // A column with both signs and a zero. The largest magnitude is 8.
  const pluss::Series v = pluss::make_series({-8.0, -2.0, 0.0, 1.0, 4.0, 6.0}, {6}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  // maxabs divides by max(|value|) = 8. Signs are kept, zero stays zero, and
  // every result lands in [-1, 1]. The stage factory and the standalone object
  // take the same (single-field) option struct.
  const lymph::prep::Stage staged = lymph::prep::stage::maxabs_scaler();
  std::cout << "staged is a MaxAbsScaler stage: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::MaxAbsScaler) << "\n\n";

  std::cout << lymph::prep::MaxAbsScaler{}.learn_map(frame);

  return 0;
}
```

Output

```text
staged is a MaxAbsScaler stage: true

=============
     | v     
-------------
0    | -1    
1    | -0.25 
2    |  0    
3    |  0.125
4    |  0.5  
5    |  0.75 
-------------
[6 cs x 1 feat]
```

### Example 3

maxabs on the numeric column, one-hot on the categorical one, chained. Fully
qualified factory spelling.

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"region", Slate(region)}});

  // maxabs on the numeric column, one-hot on the categorical one, chained.
  // Fully qualified factory spelling.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::maxabs_scaler(),
               lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  const DataFrame review({
      {"amount", Slate(amount)},
      {"region", Slate(region)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
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

- `prep::MaxAbsScaler` — the standalone twin (`prep::scaling`).
- [`options::MaxAbsScaler`](../scaling/MaxAbsScalerOptions.md) — the full option
  reference.
- [`prep::stage::minmax_scaler`](minmax_scaler.md) — a bounded range with an
  arbitrary floor and ceiling.
- [`prep::stage::standard_scaler`](standard_scaler.md) — zero mean and unit
  variance.
- [`Model::append`](../../models/Model/append.md),
  [`Model::prepare`](../../models/Model/prepare.md) — where a stage is attached.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
