<!-- pluss-hand-authored -->

# `lymph::prep::stage::ordinal_encoder`

[Back to `prep::stage`](../stage_family.md)

`ordinal_encoder` builds an attachable [`prep::Stage`](Stage.md) that maps each routed category to a small integer code — 1-based by default, with 0 reserved for a category unseen at fit time. It is the stage counterpart of the standalone `prep::OrdinalEncoder`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::ordinal_encoder(
    lymph::options::OrdinalEncoder options = {});
```

## What It Does

The call records the transform kind and options — no fitting yet. Once the owning plan fits it, the set of categories in each routed column is recorded and each value is replaced by that category's position. Unlike [`one_hot_encoder`](one_hot_encoder.md) the column count does not change: one integer column in, one integer column out. Reach for it when a downstream step wants a compact integer code and the categories have no natural order to preserve (a tree split handles them fine; a linear model reads the codes as a magnitude, which is usually not what you want — prefer one-hot there).

### Option fields

`std::int64_t max_categories = 256`
: Cap on distinct categories per column; `overflow` decides what happens past it.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: `KeepMostFrequent` keeps the top-N and folds the rest into the unseen slot; `Error` rejects the column; `SkipFeature` drops it.

`bool include_unknown_bucket = true`
: Reserve code 0 for categories not seen at fit time.

`bool one_based = true`
: Codes start at 1 (0 = unseen). Set `false` to start at 0 (Example 2).

## Parameters

`lymph::options::OrdinalEncoder options`
: The encoder configuration. Defaulted, so `ordinal_encoder()` is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::OrdinalEncoder`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A cardinality above `max_categories` raises at fit time only when `overflow` is `Error`.
- An unseen category at replay time maps to code 0 when `include_unknown_bucket` is set, otherwise it is reported.

## Practical Examples

### Example 1

The default stage, attached to a model. The routed `region` column is integer-coded behind the scenes.

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

  // Three region categories plus one numeric feature. The default stage maps
  // each routed category to a small integer code (1-based; 0 is reserved for
  // an unseen category).
  const pluss::Series spend = pluss::make_series({120.0, 200.0, 90.0, 260.0, 140.0, 300.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 1, 0, 1, 0, 1}, {6}, f64);
  const DataFrame x({{"spend", Thread(spend)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::ordinal_encoder(),
               lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)},
                          {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

================================
     | region  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       1       1        
2    | 2       0       0        
3    | 0       1       1        
4    | 1       0       0        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

`one_based` shown through the standalone `prep::OrdinalEncoder`. `true` (default) starts codes at 1 and reserves 0 for unseen; `false` starts at 0.

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

  // The expansion made visible through the standalone prep::OrdinalEncoder.
  // one_based = true (default): codes start at 1, 0 marks unseen. Set
  // one_based = false to start at 0.
  const pluss::Series tier = pluss::make_series({10, 20, 30, 10, 20, 30}, {6}, f64);
  const DataFrame frame({{"tier", Thread(tier)}});

  lymph::prep::OrdinalEncoder one_based{lymph::options::OrdinalEncoder{}};
  one_based.features({"tier"});
  std::cout << "one_based = true:\n" << one_based.learn_map(frame) << "\n";

  lymph::options::OrdinalEncoder zero{};
  zero.one_based = false;
  lymph::prep::OrdinalEncoder zero_based{zero};
  zero_based.features({"tier"});
  std::cout << "one_based = false:\n" << zero_based.learn_map(frame);

  return 0;
}
```

Output

```text
one_based = true:
===========
     | tier
-----------
0    | 1   
1    | 2   
2    | 3   
3    | 1   
4    | 2   
5    | 3   
-----------
[6 cs x 1 feat]
one_based = false:
===========
     | tier
-----------
0    | 0   
1    | 1   
2    | 2   
3    | 0   
4    | 1   
5    | 2   
-----------
[6 cs x 1 feat]
```

### Example 3

Ordinal on the categorical column, standard scaling on the numeric one, chained.

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

  // Ordinal on the categorical column, standard scaling on the numeric one,
  // chained. Fully qualified factory spelling.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::ordinal_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"amount", Thread(amount)}, {"region", Thread(region)},
                          {"actual", Thread(y)}, {"predicted", Thread(model.infer(x))}});

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

- `prep::OrdinalEncoder` — the standalone twin (`prep::encoding`).
- [`prep::stage::one_hot_encoder`](one_hot_encoder.md) — indicator columns instead of an integer code.
- [`prep::stage::label_encoder`](label_encoder.md) — the same coding with a 0-based default.
- [`Model::append`](../../models/Model/append.md), [`Model::prepare`](../../models/Model/prepare.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
