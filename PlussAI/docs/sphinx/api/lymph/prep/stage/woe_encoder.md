<!-- pluss-hand-authored -->

# `lymph::prep::stage::woe_encoder`

[Back to `prep::stage`](../stage_family.md)

`woe_encoder` builds a **supervised** attachable [`prep::Stage`](Stage.md) for a binary `0/1` target: each category is replaced by its weight of evidence — the log-odds contribution of that category toward class 1.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::woe_encoder(
    lymph::options::WeightOfEvidence options = {});
```

## What It Does

Fitting computes, per category, `ln( ((pos_c + r)/(P + 2r)) / ((neg_c + r)/(N + 2r)) )`, where `P`/`N` are the total positives/negatives and `r` is a Laplace term. A category that skews toward class 1 gets a positive value, toward class 0 a negative one, and a balanced or unseen category gets `0`. One numeric column in, one out. It is the standard encoding in scorecard-style modelling: monotone in the target rate, additive on the log-odds scale, and easy to read. Binary targets only.

### Option fields

`double regularization = 1.0`
: The Laplace term `r`. It keeps the log-odds finite for a category that is all-positive or all-negative, and larger values shrink every category toward `0` (Example 2).

`std::int64_t max_categories = 4096`
: Cap on distinct categories per column.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: Behaviour past `max_categories`.

## Parameters

`lymph::options::WeightOfEvidence options`
: The encoder configuration. Defaulted. `lymph::prep::WOEEncoderOptions` is the family-scoped alias.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::WOEEncoder`, with `contract().target_aware == true`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A target that is not strictly `0`/`1` is reported at fit time.
- Attaching it to a target-free transform flow is reported when that flow is fitted.

## Practical Examples

### Example 1

The default stage, attached to a model. `region` becomes its log-odds column.

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

  // woe_encoder is supervised and strictly for a binary 0/1 target. Each
  // category becomes its log-odds contribution: positive if the category skews
  // toward class 1, negative toward class 0.
  const pluss::Series region = pluss::make_series({0,0,1,1,2,2,3,3,0,2}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::woe_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
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
1    | 0       0       0        
2    | 1       0       0        
3    | 1       0       0        
4    | 2       1       1        
5    | 2       1       1        
6    | 3       1       1        
7    | 3       1       1        
8    | 0       0       0        
9    | 2       1       1        
--------------------------------
[10 cs x 3 feat]
```

### Example 2

`regularization` mainly guards degenerate categories (all one class); on this data it barely moves the fit. It is a stability control, not an accuracy knob.

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

  // regularization is a Laplace term that keeps the log-odds finite for a
  // category that is all-positive or all-negative. Larger = more shrinkage
  // toward 0 (neutral).
  const pluss::Series region = pluss::make_series({0,0,0,1,1,1,2,2,2,2}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Thread(region)}});

  for (double r : {0.5, 20.0}) {
    lymph::options::WeightOfEvidence o{};
    o.regularization = r;
    lymph::models::Model model = lymph::models::logistic_regression();
    model.append(lymph::prep::stage::woe_encoder(o), lymph::prep::route::names({"region"}));
    model.learn_(x, y);
    std::cout << "regularization = " << r << " -> accuracy "
              << model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>() << "\n";
  }

  return 0;
}
```

Output

```text
regularization = 0.5 -> accuracy 0.9
regularization = 20 -> accuracy 0.9
```

### Example 3

WoE on the categorical `channel`, robust scaling on the numeric `bal`.

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

  const pluss::Series channel = pluss::make_series({0,1,2,0,1,2,3,3,0,1}, {10}, f64);
  const pluss::Series bal = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.,9.,10.}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,1,0,0,1,1,1,0,0}, {10}, f64);
  const DataFrame x({{"channel", Thread(channel)}, {"bal", Thread(bal)}});

  // WoE on the categorical channel, robust scaling on the balance column.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::robust_scaler(), lymph::prep::route::names({"bal"}))
       .append(lymph::prep::stage::woe_encoder(), lymph::prep::route::names({"channel"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"channel", Thread(channel)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

=================================
     | channel  actual  predicted
---------------------------------
0    | 0        0       0        
1    | 1        0       0        
2    | 2        1       1        
3    | 0        0       0        
4    | 1        0       0        
5    | 2        1       1        
6    | 3        1       1        
7    | 3        1       1        
8    | 0        0       0        
9    | 1        0       0        
---------------------------------
[10 cs x 3 feat]
```

## See Also

- [`prep::stage::target_encoder`](target_encoder.md) — mean-of-target, works for regression too.
- [`Model::learn`](../../models/Model/learn.md) — supplies the target.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
