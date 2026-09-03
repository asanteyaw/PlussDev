<!-- pluss-hand-authored -->

# `lymph::prep::stage::label_encoder`

[Back to `prep::stage`](../stage_family.md)

`label_encoder` builds an attachable [`prep::Stage`](Stage.md) that integer-codes a routed column, the same operation as [`ordinal_encoder`](ordinal_encoder.md) but 0-based by default — the shape a downstream step expects for a label-style column indexed `0..K-1`.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::label_encoder(
    lymph::options::LabelEncoder options = {});
```

## What It Does

Fitting records the categories in the routed column; replay swaps each value for its 0-based position. The historical name comes from encoding a target label column; as a routed stage it simply integer-codes whatever column the route selects. Column count is unchanged.

### Option fields

`std::int64_t max_categories = 256`, `overflow`, `include_unknown_bucket = true`
: As for [`ordinal_encoder`](ordinal_encoder.md).

`bool one_based = false`
: Codes start at 0. This is the only default that differs from `ordinal_encoder`.

## Parameters

`lymph::options::LabelEncoder options`
: The encoder configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::LabelEncoder`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- Cardinality and unknown-category handling match [`ordinal_encoder`](ordinal_encoder.md).

## Practical Examples

### Example 1

The default stage, attached to a model. `grade` is coded `0..K-1` before the estimator sees it.

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

  // label_encoder integer-codes a routed column, like ordinal_encoder but with
  // one_based = false by default -- the shape used for a target-style label
  // column that a downstream step expects as 0..K-1.
  const pluss::Series grade = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series score = pluss::make_series({0.2, 0.5, 0.9, 0.3, 0.6, 0.8}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"grade", Thread(grade)}, {"score", Thread(score)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::label_encoder(), lymph::prep::route::names({"grade"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"grade", Thread(grade)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

===============================
     | grade  actual  predicted
-------------------------------
0    | 0      0       0        
1    | 1      0       0        
2    | 2      1       1        
3    | 0      0       0        
4    | 1      1       1        
5    | 2      1       1        
-------------------------------
[6 cs x 3 feat]
```

### Example 2

Standalone, showing the 0-based codes. `label_encoder` and `ordinal_encoder` share their option struct; only the `one_based` default differs.

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

  // Standalone, showing the 0-based codes. label_encoder and ordinal_encoder
  // share their option struct; the difference is the one_based default.
  const pluss::Series band = pluss::make_series({5, 9, 5, 1, 9, 1}, {6}, f64);
  const DataFrame frame({{"band", Thread(band)}});

  lymph::prep::LabelEncoder enc{lymph::options::LabelEncoder{}};
  enc.features({"band"});
  std::cout << enc.learn_map(frame);

  return 0;
}
```

Output

```text
===========
     | band
-----------
0    | 1   
1    | 2   
2    | 1   
3    | 0   
4    | 2   
5    | 0   
-----------
[6 cs x 1 feat]
```

### Example 3

Label-code the categorical column, minmax-scale the numeric one, passed as one batch.

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
  const pluss::Series grade = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"grade", Thread(grade)}});

  // label-encode the ordinal-ish column, minmax-scale the numeric one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append({
      {lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"amount"})},
      {lymph::prep::stage::label_encoder(), lymph::prep::route::names({"grade"})},
  });
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"grade", Thread(grade)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

===============================
     | grade  actual  predicted
-------------------------------
0    | 0      0       0        
1    | 1      0       0        
2    | 2      0       0        
3    | 0      1       1        
4    | 1      1       1        
5    | 2      1       1        
-------------------------------
[6 cs x 3 feat]
```

## See Also

- `prep::LabelEncoder` — the standalone twin.
- [`prep::stage::ordinal_encoder`](ordinal_encoder.md) — same coding, 1-based default.
- [`Model::append`](../../models/Model/append.md), [`Model::prepare`](../../models/Model/prepare.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
