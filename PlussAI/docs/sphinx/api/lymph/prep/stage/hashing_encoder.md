<!-- pluss-hand-authored -->

# `lymph::prep::stage::hashing_encoder`

[Back to `prep::stage`](../stage.md)

`hashing_encoder` builds an attachable [`prep::Stage`](Stage.md) that maps each routed category into one of `n_components` fixed buckets with a multiplicative hash and emits an `n_components`-wide indicator. The output width is independent of how many categories appear, and no vocabulary is stored.

## Signature

```cpp
lymph::prep::Stage lymph::prep::stage::hashing_encoder(
    lymph::options::HashingEncoder options = {});
```

## What It Does

There is nothing to learn beyond the frame shape — the bucket is a pure function of the value and the seed. Replay emits `n_components` columns per routed column, with a `1` in the hashed bucket. Because it is stateless, an unseen category simply hashes somewhere; the cost is that two distinct categories can land in the same bucket (a collision). Reach for it when cardinality is very large or unbounded, or in a streaming setting where you cannot fix a vocabulary up front. A missing value produces an all-zero row.

### Option fields

`std::int64_t n_components = 8`
: Number of output columns per routed column. More buckets means fewer collisions and a wider frame.

`std::uint64_t seed = 0`
: Perturbs the hash. Change it to get a different bucket assignment for the same categories.

## Parameters

`lymph::options::HashingEncoder options`
: The encoder configuration. Defaulted.

## Returns

A `lymph::prep::Stage` carrying `TransformKind::HashingEncoder`. Cheap to copy; holds no fitted state.

## Errors

- The factory itself does not throw.
- `n_components` below 1 is rejected at fit time.

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

  // hashing_encoder maps each category into one of n_components fixed buckets.
  // Output width is fixed regardless of how many categories appear, and unseen
  // categories need no special handling.
  const pluss::Series region = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const pluss::Series spend  = pluss::make_series({120.,200.,90.,260.,140.,300.,110.,240.}, {8}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,0,0,0}, {8}, f64);
  const DataFrame x({{"region", Slate(region)}, {"spend", Slate(spend)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::hashing_encoder(), lymph::prep::route::names({"region"}));
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

Standalone with `n_components = 4`. Each row lands in exactly one bucket; category 2 collides with category 0 in bucket `c_0` — expected for a hash.

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

  // Standalone with n_components = 4. Each row lands in exactly one bucket;
  // two distinct categories can collide into the same bucket by design.
  const pluss::Series c = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const DataFrame frame({{"c", Slate(c)}});

  lymph::options::HashingEncoder o{};
  o.n_components = 4;
  lymph::prep::HashingEncoder enc{o};
  enc.features({"c"});
  const DataFrame out = enc.learn_map(frame);
  std::cout << "5 categories -> " << out.width() << " fixed buckets\n" << out;

  return 0;
}
```

Output

```text
5 categories -> 4 fixed buckets
=============================
     | c_0   c_1   c_2   c_3 
-----------------------------
0    | 1     0     0     0   
1    | 0     0     1     0   
2    | 1     0     0     0   
3    | 0     0     0     1   
4    | 0     1     0     0   
5    | 1     0     0     0   
6    | 0     0     1     0   
7    | 1     0     0     0   
-----------------------------
[8 cs x 4 feat]
```

### Example 3

Scale the numeric column, hash the categorical one into six buckets.

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

  lymph::options::HashingEncoder o{};
  o.n_components = 6;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}))
       .append(lymph::prep::stage::hashing_encoder(o), lymph::prep::route::names({"region"}));
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

- [`prep::stage::basen_encoder`](basen_encoder.md) — compact, collision-free, but needs a vocabulary.
- [`prep::stage::one_hot_encoder`](one_hot_encoder.md) — faithful, width grows with cardinality.
- [`Model::append`](../../models/Model/append.md), [`Model::prepare`](../../models/Model/prepare.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
