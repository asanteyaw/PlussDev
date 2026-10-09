<!-- pluss-hand-authored -->

# `lymph::prep::stage::basen_encoder`

[Back to `prep::stage`](../stage.md)

`basen_encoder` builds an attachable [`prep::Stage`](Stage.md) that integer-codes each routed category and then writes that code in radix `base` across a fixed set of digit columns. `base = 2` is the popular *binary encoder*. It trades one-hot's one column per category for `ceil(log_base(K + 1))` columns.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::basen_encoder(
    lymph::options::BaseNEncoder options = {});
```

## What It Does

Fitting records the categories (codes `1..K`, with `0` reserved for an unseen or missing value) and how many radix-`base` digits `K` needs. Replay emits that many `0..base-1` columns per routed column. It is the middle ground between [`ordinal_encoder`](ordinal_encoder.md) (one column, but a linear model reads the code as a magnitude) and [`one_hot_encoder`](one_hot_encoder.md) (faithful, but `K` columns wide): a compact multi-column code a linear model can use without the full width.

### Option fields

`std::int64_t base = 2`
: The radix. `2` gives a binary code; a larger base uses fewer, wider digit columns (Example 2).

`std::int64_t max_categories = 1024`
: Cap on distinct categories per column.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: What to do past `max_categories` — keep the most frequent, `Error`, or `SkipFeature`.

## Parameters

`lymph::options::BaseNEncoder options`
: The encoder configuration. Defaulted, so `basen_encoder()` (binary) is the common call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::BaseNEncoder`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- `base` below 2 is rejected at fit time.
- A cardinality above `max_categories` raises at fit time only when `overflow` is `Error`.

## Practical Examples

### Example 1

The default (binary) stage, attached to a model. Five region categories become three digit columns.

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

  // Five region categories become 3 binary-digit columns (base 2, the default):
  // ceil(log2(5+1)) = 3, versus the 5 columns one-hot would spend.
  const pluss::Series region = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const pluss::Series spend  = pluss::make_series({120.,200.,90.,260.,140.,300.,110.,240.}, {8}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,0,0,0}, {8}, f64);
  const DataFrame x({{"region", Slate(region)}, {"spend", Slate(spend)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::basen_encoder(), lymph::prep::route::names({"region"}));
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
1    | 1       0       0        
2    | 2       0       0        
3    | 3       1       1        
4    | 4       1       1        
5    | 0       0       0        
6    | 1       0       0        
7    | 2       0       0        
--------------------------------
[8 cs x 3 feat]
```

### Example 2

The digit layout, standalone. Codes are `1..5` (0 = unseen). `base = 2` gives a 3-column binary code; `base = 3` packs the same five categories into 2 columns.

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

  // Standalone, showing the digit layout. Codes are 1..K (0 = unseen). base 2
  // gives a binary code; base 3 packs the same 5 categories into 2 columns.
  const pluss::Series c = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const DataFrame frame({{"c", Slate(c)}});

  lymph::prep::BaseNEncoder binary{lymph::options::BaseNEncoder{}};   // base 2
  binary.features({"c"});
  std::cout << "base = 2:\n" << binary.learn_map(frame) << "\n";

  lymph::options::BaseNEncoder b3{};
  b3.base = 3;
  lymph::prep::BaseNEncoder ternary{b3};
  ternary.features({"c"});
  std::cout << "base = 3:\n" << ternary.learn_map(frame);

  return 0;
}
```

Output

```text
base = 2:
=======================
     | c_0   c_1   c_2 
-----------------------
0    | 1     0     0   
1    | 0     1     0   
2    | 1     1     0   
3    | 0     0     1   
4    | 1     0     1   
5    | 1     0     0   
6    | 0     1     0   
7    | 1     1     0   
-----------------------
[8 cs x 3 feat]
base = 3:
=================
     | c_0   c_1 
-----------------
0    | 1     0   
1    | 2     0   
2    | 0     1   
3    | 1     1   
4    | 2     1   
5    | 1     0   
6    | 2     0   
7    | 0     1   
-----------------
[8 cs x 2 feat]
```

### Example 3

Scale the numeric column, base-N encode a higher-cardinality categorical one.

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

  const pluss::Series amount = pluss::make_series({1200.,1800.,3400.,4100.,5200.,6000.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,3,4,5}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"region", Slate(region)}});

  // scale the numeric column, base-N encode the high-cardinality categorical one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::basen_encoder(), lymph::prep::route::names({"region"}));
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
1    | 1       0       0        
2    | 2       0       0        
3    | 3       1       1        
4    | 4       1       1        
5    | 5       1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::one_hot_encoder`](one_hot_encoder.md) — faithful but `K` columns wide.
- [`prep::stage::hashing_encoder`](hashing_encoder.md) — fixed width, no vocabulary, collisions allowed.
- [`prep::stage::ordinal_encoder`](ordinal_encoder.md) — a single integer column.
- [`Model::append`](../../models/Model/append.md), [`Model::prepare`](../../models/Model/prepare.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
