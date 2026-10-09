<!-- pluss-hand-authored -->

# `lymph::prep::stage::threshold_binarizer`

[Back to `prep::stage`](../stage.md)

`threshold_binarizer` builds an attachable [`prep::Stage`](Stage.md) that turns a routed numeric column into a two-valued flag about a cut point. It is stateless — the threshold is a fixed option, nothing is learned.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::threshold_binarizer(
    lymph::options::ThresholdBinarizer options = {});
```

## What It Does

Replay compares each value to `threshold` and emits `low_value` below or at it and `high_value` above. Use it to hand a model a crisp "is this over the line?" feature — an eligibility cutoff, a sign test, a domain rule — without letting the raw magnitude in. Because it keeps no fitted state, the same stage behaves identically on every frame.

### Option fields

`double threshold = 0.0`
: The cut point. A value greater than this becomes `high_value`; a value at or below it becomes `low_value`.

`double low_value = 0.0`, `double high_value = 1.0`
: The two output values. Default `0` / `1`; set them to `-1` / `+1` for a sign feature (Example 2).

## Parameters

`lymph::options::ThresholdBinarizer options`
: The transform configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::ThresholdBinarizer`, with `contract().stateful == false`. Cheap to copy.

## Errors

- The factory itself does not throw.
- `low_value == high_value` produces a constant column.

## Practical Examples

### Example 1

A stage with `threshold = 5.0`, attached to a model. `balance` collapses to a below/above flag.

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

  // threshold_binarizer turns a numeric column into a 0/1 flag about a cut
  // point. It is stateless -- the threshold is a fixed option, not learned.
  const pluss::Series balance = pluss::make_series({-50., -5., 0., 10., 40., 120.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"balance", Slate(balance)}});

  lymph::options::ThresholdBinarizer o{};
  o.threshold = 5.0;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::threshold_binarizer(o), lymph::prep::route::names({"balance"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"balance", Slate(balance)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

=================================
     | balance  actual  predicted
---------------------------------
0    | -50      0       0        
1    | -5       0       0        
2    |  0       0       0        
3    |  10      1       1        
4    |  40      1       1        
5    |  120     1       1        
---------------------------------
[6 cs x 3 feat]
```

### Example 2

`low_value` / `high_value`, standalone. Default `0` / `1`, or `-1` / `+1` for a sign feature.

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

  // threshold is the cut; low_value / high_value are what a below / above value
  // becomes. Default is 0 / 1; here a second binarizer emits -1 / +1.
  const pluss::Series v = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::ThresholdBinarizer zero_one{};                       // threshold 0, 0/1
  lymph::options::ThresholdBinarizer signed_out{};
  signed_out.low_value = -1.0;
  signed_out.high_value = 1.0;

  std::cout << "0 / 1 about 0:\n"  << lymph::prep::ThresholdBinarizer{zero_one}.learn_map(frame) << "\n";
  std::cout << "-1 / +1 about 0:\n" << lymph::prep::ThresholdBinarizer{signed_out}.learn_map(frame);

  return 0;
}
```

Output

```text
0 / 1 about 0:
===========
     | v   
-----------
0    | 0   
1    | 0   
2    | 0   
3    | 1   
4    | 1   
5    | 1   
-----------
[6 cs x 1 feat]
-1 / +1 about 0:
===========
     | v   
-----------
0    | -1  
1    | -1  
2    | -1  
3    |  1  
4    |  1  
5    |  1  
-----------
[6 cs x 1 feat]
```

### Example 3

Binarize the score column at `0.5`, one-hot the categorical one.

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

  const pluss::Series score = pluss::make_series({0.1, 0.3, 0.55, 0.7, 0.9, 0.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,1,1,1,0}, {6}, f64);
  const DataFrame x({{"score", Slate(score)}, {"region", Slate(region)}});

  lymph::options::ThresholdBinarizer o{};
  o.threshold = 0.5;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::threshold_binarizer(o), lymph::prep::route::names({"score"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"score", Slate(score)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

===============================
     | score  actual  predicted
-------------------------------
0    | 0.1    0       0        
1    | 0.3    0       0        
2    | 0.55   1       1        
3    | 0.7    1       1        
4    | 0.9    1       1        
5    | 0.4    0       0        
-------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::stage::custom`](custom.md) — for a multi-way bucketing rule.
- `prep::ThresholdBinarizer` — the standalone twin.
- [`Model::append`](../../models/Model/append.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
