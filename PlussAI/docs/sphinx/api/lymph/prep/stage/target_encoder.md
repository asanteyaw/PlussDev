<!-- pluss-hand-authored -->

# `lymph::prep::stage::target_encoder`

[Back to `prep::stage`](../stage_family.md)

`target_encoder` builds a **supervised** attachable [`prep::Stage`](Stage.md): each category is replaced by a smoothed mean of the training target for that category. The target reaches the stage automatically during [`Model::learn`](../../models/Model/learn.md) — it is not a target-free transform.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::target_encoder(
    lymph::options::TargetEncoder options = {});
```

## What It Does

Fitting computes, per category, `(Σy + smoothing·prior) / (count + smoothing)`, where `prior` is the global target mean. A category with many rows converges to its own mean; a rare one is pulled toward the prior. Replay swaps each value for its fitted number; an unseen category gets the prior. One numeric column in, one out. It is the most information-dense encoding for a high-cardinality categorical when a target is available — at the cost of leakage risk, which the smoothing and, in a real pipeline, cross-validated fitting are there to contain. Appropriate for regression and binary (0/1) targets.

### Option fields

`double smoothing = 10.0`
: Pseudo-observations of the prior mixed into every category estimate. Larger shrinks rare categories harder toward the global mean (Example 2).

`std::int64_t max_categories = 4096`
: Cap on distinct categories per column.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: Behaviour past `max_categories`.

## Parameters

`lymph::options::TargetEncoder options`
: The encoder configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::TargetEncoder`, with `contract().target_aware == true`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- Attaching it to a target-free transform flow (rather than `Model::learn(x, y)`) is reported when that flow is fitted.
- A cardinality above `max_categories` raises at fit time only when `overflow` is `Error`.

## Practical Examples

### Example 1

The default stage, attached to a model. The fitted target reaches the stage during `learn_`; `region` becomes a single mean-of-target column.

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

  // target_encoder is supervised: attach it and the fitted target reaches the
  // stage during Model::learn(x, y). Each category is replaced by a smoothed
  // mean of the target for that category.
  const pluss::Series region = pluss::make_series({0,0,1,1,2,2,3,3,0,2}, {10}, f64);
  const pluss::Series noise  = pluss::make_series({.1,.9,.2,.8,.3,.7,.4,.6,.5,.5}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Thread(region)}, {"noise", Thread(noise)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::target_encoder(), lymph::prep::route::names({"region"}));
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

`smoothing` sets how hard a category is pulled toward the global mean. On this cleanly separable signal both settings still fit; the difference shows on noisy, rare categories.

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

  // smoothing controls how hard a rare category is pulled toward the global
  // mean. A larger value shrinks more. Two models, same data, different
  // smoothing -- both still fit this clean signal.
  const pluss::Series region = pluss::make_series({0,0,0,0,1,1,1,1,2,3}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,1,0}, {10}, f64);
  const DataFrame x({{"region", Thread(region)}});

  for (double s : {1.0, 50.0}) {
    lymph::options::TargetEncoder o{};
    o.smoothing = s;
    lymph::models::Model model = lymph::models::logistic_regression();
    model.append(lymph::prep::stage::target_encoder(o), lymph::prep::route::names({"region"}));
    model.learn_(x, y);
    std::cout << "smoothing = " << s << " -> accuracy "
              << model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>() << "\n";
  }

  return 0;
}
```

Output

```text
smoothing = 1 -> accuracy 1
smoothing = 50 -> accuracy 1
```

### Example 3

A high-cardinality `city` next to a numeric feature: target-encode the city to one informative column, scale the numeric one.

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

  // A high-cardinality categorical alongside a numeric feature: target-encode
  // the category to a single informative column, scale the numeric one.
  const pluss::Series city = pluss::make_series({0,1,2,3,4,5,6,7,0,3}, {10}, f64);
  const pluss::Series size = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.,1.,4.}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,0}, {10}, f64);
  const DataFrame x({{"city", Thread(city)}, {"size", Thread(size)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"size"}))
       .append(lymph::prep::stage::target_encoder(), lymph::prep::route::names({"city"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"city", Thread(city)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

==============================
     | city  actual  predicted
------------------------------
0    | 0     0       0        
1    | 1     0       0        
2    | 2     0       0        
3    | 3     0       0        
4    | 4     1       1        
5    | 5     1       1        
6    | 6     1       1        
7    | 7     1       1        
8    | 0     0       0        
9    | 3     0       0        
------------------------------
[10 cs x 3 feat]
```

## See Also

- [`prep::stage::woe_encoder`](woe_encoder.md) — the log-odds sibling for binary targets.
- [`prep::stage::frequency_encoder`](frequency_encoder.md) — an unsupervised count-based alternative.
- [`Model::learn`](../../models/Model/learn.md) — supplies the target the stage fits on.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
