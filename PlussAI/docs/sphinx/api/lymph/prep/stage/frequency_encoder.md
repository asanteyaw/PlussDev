<!-- pluss-hand-authored -->

# `lymph::prep::stage::frequency_encoder`

[Back to `prep::stage`](../stage.md)

`frequency_encoder` builds an attachable [`prep::Stage`](Stage.md) that replaces each category with how often it occurred at fit time, as a proportion in `[0, 1]`. It is the stage counterpart of the standalone `prep::FrequencyEncoder`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::frequency_encoder(
    lymph::options::FrequencyEncoder options = {});
```

## What It Does

Fitting counts each category in the routed column; replay swaps each value for its observed relative frequency. One numeric column in, one out. It is a cheap way to give a model a rough popularity signal for a high-cardinality column without the width of one-hot — but two different categories that happen to be equally common collapse to the same value.

### Option fields

`std::int64_t max_categories = 50`
: Cap on tracked categories; `overflow` decides the rest.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: As for the other categorical encoders.

`double unknown_frequency = 0.0`
: The value an unseen category receives at replay time.

## Parameters

`lymph::options::FrequencyEncoder options`
: The encoder configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::FrequencyEncoder`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A cardinality above `max_categories` raises at fit time only when `overflow` is `Error`.

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

  // frequency_encoder replaces each category with how often it was seen at fit
  // time (as a proportion). Rare categories get small values, common ones large.
  const pluss::Series region = pluss::make_series({0, 0, 0, 1, 1, 2}, {6}, f64);
  const pluss::Series spend = pluss::make_series({120.0, 200.0, 90.0, 260.0, 140.0, 300.0}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"region", Slate(region)}, {"spend", Slate(spend)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::frequency_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

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
1    | 0       0       0        
2    | 0       0       0        
3    | 1       1       1        
4    | 1       1       1        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

Standalone view: region 0 appears 3/6, region 1 2/6, region 2 1/6.

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

  // Standalone view: region 0 appears 3/6, region 1 2/6, region 2 1/6.
  const pluss::Series region = pluss::make_series({0, 0, 0, 1, 1, 2}, {6}, f64);
  const DataFrame frame({{"region", Slate(region)}});

  lymph::prep::FrequencyEncoder enc{lymph::options::FrequencyEncoder{}};
  enc.features({"region"});
  std::cout << enc.learn_map(frame);

  return 0;
}
```

Output

```text
===================
     | region      
-------------------
0    | 0.5         
1    | 0.5         
2    | 0.5         
3    | 0.3333333333
4    | 0.3333333333
5    | 0.1666666667
-------------------
[6 cs x 1 feat]
```

### Example 3

Frequency-encode the categorical column, standard-scale the numeric one.

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
  const pluss::Series region = pluss::make_series({0, 0, 1, 1, 2, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::frequency_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

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
1    | 0       0       0        
2    | 1       0       0        
3    | 1       1       1        
4    | 2       1       1        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- `prep::FrequencyEncoder` — the standalone twin.
- [`prep::stage::target_encoder`](target_encoder.md) — a supervised alternative that uses the target, not raw counts.
- [`Model::append`](../../models/Model/append.md), [`Model::prepare`](../../models/Model/prepare.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
