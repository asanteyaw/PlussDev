<!-- pluss-hand-authored -->

# `lymph::prep::stage::one_hot_encoder`

[Back to `prep::stage`](../stage_family.md)

`one_hot_encoder` builds an attachable [`prep::Stage`](Stage.md) that expands a
routed categorical column into one indicator column per category. It is the
stage-shaped counterpart of the standalone
[`prep::OneHotEncoder`](../encoding/OneHotEncoder.md).

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::one_hot_encoder(
    lymph::options::OneHotEncoder options = {});
```

`lymph::options::OneHotEncoder` is the canonical spelling of the option struct;
`lymph::prep::OneHotEncoderOptions` is the family-scoped alias for the same type.

## What It Does

The call returns a `Stage` descriptor recording the transform kind
(`OneHotEncoder`) and the options. It fits nothing on its own. Once paired with a
[`prep::Route`](../route/Route.md) and attached to a plan
([`Model::append`](../../models/Model/append.md),
[`Model::prepare`](../../models/Model/prepare.md), or a
`prep::FeatureTransformer` branch), fitting records the set of categories seen in
each routed column. Replay then emits, per routed column, one `0/1` column for
each recorded category, plus — when `include_unknown_bucket` is set — one extra
column that absorbs categories not seen at fit time.

The routed column's original values are replaced by the indicator columns, so a
one-hot stage **increases the feature count** (its
[`contract().changes_feature_count`](Stage.md#contract) is `true`).

### Option fields

The three fields of `lymph::options::OneHotEncoder`:

`std::int64_t max_categories = 20`
: Upper bound on how many real category columns one routed column may produce.
  Guards against a high-cardinality column (an id, a free-text field) exploding
  the frame width.

`lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`
: What to do when a column has more than `max_categories` distinct values.
  `KeepMostFrequent` keeps the `max_categories` most common and drops the tail
  (Example 3); `Error` rejects the column; `SkipFeature` drops the whole column.

`bool include_unknown_bucket = true`
: Reserve one extra column per routed column for categories that were dropped by
  `overflow` or never seen at fit time. Whether an unseen value at replay time
  *lands* in that bucket or raises is a separate, inference-time choice
  (`options::Infer::unknown_categorical`, default `Error`); set it to
  `MissingBucket` to route unseen values into the reserved column (Example 3).

The full field reference lives on the
[`options::OneHotEncoder` page](../encoding/OneHotEncoderOptions.md); it is the
same struct the standalone `prep::OneHotEncoder` takes.

## Parameters

`lymph::options::OneHotEncoder options`
: The encoder configuration. Defaulted, so `one_hot_encoder()` is the common
  call.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::OneHotEncoder` and the given
options. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- A routed column whose cardinality exceeds `max_categories` raises at fit time
  only when `overflow` is `Error`.
- An unseen category at replay time raises unless the inference-time
  `unknown_categorical` policy is `MissingBucket` or `Ignore`.

## Practical Examples

### Example 1

The default stage, attached to a model. The routed `region` column is expanded
behind the scenes; the caller still passes the raw frame.

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

  // A categorical region code and one numeric feature.
  const pluss::Series spend = pluss::make_series({120.0, 200.0, 90.0, 260.0, 140.0, 300.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 1, 0, 1, 0, 1}, {6}, f64);
  const DataFrame x({{"spend", Thread(spend)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage expands the routed categorical column into one indicator
  // column per category. No options argument means a default-constructed
  // lymph::options::OneHotEncoder.
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"spend", Thread(spend)},
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
stage_count = 1
trained     = true

=======================================
     | spend  region  actual  predicted
---------------------------------------
0    | 120    0       0       0        
1    | 200    1       1       1        
2    | 90     2       0       0        
3    | 260    0       1       1        
4    | 140    1       0       0        
5    | 300    2       1       1        
---------------------------------------
[6 cs x 4 feat]
```

### Example 2

The expansion made visible through the standalone `prep::OneHotEncoder`, which
takes the same option struct. Three categories over six rows become four
columns: one per category plus the reserved unknown bucket (`region_3`, all zero
here since every value was seen). On a `DataFrame` the categorical column must be
named with `.features({...})`.

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

  // Three distinct categories over six rows.
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const DataFrame frame({{"region", Thread(region)}});

  // The stage factory and the standalone prep::OneHotEncoder take the same
  // option struct. The standalone object shows the expansion directly.
  lymph::prep::OneHotEncoder encoder{lymph::options::OneHotEncoder{}};
  encoder.features({"region"});  // name the categorical column on a DataFrame
  const DataFrame expanded = encoder.learn_map(frame);

  std::cout << "one input column -> " << expanded.width() << " indicator columns\n\n";
  std::cout << expanded;

  return 0;
}
```

Output

```text
one input column -> 4 indicator columns

=============================================
     | region_0  region_1  region_2  region_3
---------------------------------------------
0    | 1         0         0         0       
1    | 0         1         0         0       
2    | 0         0         1         0       
3    | 1         0         0         0       
4    | 0         1         0         0       
5    | 0         0         1         0       
---------------------------------------------
[6 cs x 4 feat]
```

### Example 3

`max_categories` with `overflow = KeepMostFrequent`. A four-category column is
capped at three real columns; the least frequent category is dropped, and with
the inference-time `unknown_categorical` policy set to `MissingBucket` its rows
fall into the reserved fourth column.

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

  // Four categories with clearly different frequencies: 0 appears 4x, 1 3x,
  // 2 2x, 3 once.
  const pluss::Series tier = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 2, 2, 3}, {10}, f64);
  const DataFrame frame({{"tier", Thread(tier)}});

  // max_categories caps how many real category columns are produced; overflow
  // says what happens to the rest. KeepMostFrequent keeps the top-N by count
  // (0, 1, 2 here) and drops the tail (3).
  lymph::options::OneHotEncoder capped{};
  capped.max_categories = 3;
  capped.overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent;
  capped.include_unknown_bucket = true;  // reserve a column for dropped/unseen

  // At map time, route anything not in the kept set into that reserved bucket
  // rather than raising.
  lymph::options::Infer infer{};
  infer.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;

  lymph::prep::OneHotEncoder encoder{capped};
  encoder.features({"tier"});
  const DataFrame expanded = encoder.learn_map(frame, {}, infer);

  std::cout << "4 categories, max_categories = 3  ->  " << expanded.width()
            << " columns (3 kept + 1 bucket)\n\n";
  std::cout << expanded;

  return 0;
}
```

Output

```text
4 categories, max_categories = 3  ->  4 columns (3 kept + 1 bucket)

=====================================
     | tier_0  tier_1  tier_2  tier_3
-------------------------------------
0    | 1       0       0       0     
1    | 1       0       0       0     
2    | 1       0       0       0     
3    | 1       0       0       0     
4    | 0       1       0       0     
5    | 0       1       0       0     
6    | 0       1       0       0     
7    | 0       0       1       0     
8    | 0       0       1       0     
9    | 0       0       0       1     
-------------------------------------
[10 cs x 4 feat]
```

The last column, `tier_3`, is the reserved bucket: row 9 carried the dropped
category and lands there.

### Example 4

A one-hot stage paired with its route, alongside a scaler stage, through chained
`append` and the fully qualified factory spelling.

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

  // A one-hot stage paired with its route, alongside a scaler stage, via the
  // fully qualified factory spelling.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(),
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

- [`prep::OneHotEncoder`](../encoding/OneHotEncoder.md) — the standalone twin.
- [`options::OneHotEncoder`](../encoding/OneHotEncoderOptions.md) — the full
  option reference.
- [`prep::stage::standard_scaler`](standard_scaler.md) — the numeric stage
  factory.
- [`Model::append`](../../models/Model/append.md),
  [`Model::prepare`](../../models/Model/prepare.md) — where a stage is attached.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
