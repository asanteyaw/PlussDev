# `lymph::FeatureTransformer`

## Summary

`FeatureTransformer` is the reusable branch-aware feature mapping object in the
Lymph prep family.

Use it when one preprocessing contract should coordinate several feature-local
transformations and possibly a remainder policy.

## What It Is For

`FeatureTransformer` is the right abstraction when:

- the transformation is bigger than one scaler or one encoder
- several routed branches should be composed into one reusable mapping
- the transform should be a first-class object instead of scattered helper code

This is how you keep custom feature engineering from turning into opaque manual
plumbing.

## What Makes It Different

Compared with a standalone scaler or encoder, `FeatureTransformer` can coordinate
multiple branch-local transformations under one object.

That is why it matters for more complex feature shaping:

- scale one subset
- encode another subset
- pass a remainder branch through
- keep the whole contract reusable and trainable

## Signature

```cpp
lymph::FeatureTransformer lymph::make_feature_transformer(...);
lymph::FeatureTransformer lymph::make_branch_transformer(...);
pluss::Series lymph::FeatureTransformer::learn_map(...) ;
```

## Practical Example

This page uses the exact verifier program
`lymph_prep_feature_transformer_docverify_demo.cpp`.

### Example 1

This example builds a feature transformer with two routed branches and a
pass-through remainder policy, then maps a feature batch through it.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions options{};
  options.feature_names = {"income", "region", "balance"};
  options.note_policy = lymph::DataNotePolicy::Factorize;
  options.layout = lymph::ObservationLayout::Rows;

  lymph::FeatureTransformer transformer = lymph::make_feature_transformer(
      raw,
      options,
      {
          lymph::workflow::feature::compose(
              {
                  lymph::workflow::stage::standard_scaler(),
              },
              {"income"}),
          lymph::workflow::feature::one_hot_encoder(
              lymph::prep::OneHotEncoderOptions{
                  .max_categories = 8,
                  .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
                  .include_unknown_bucket = true,
              },
              {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  const lymph::Payload payload = lymph::payload::make(raw)
                                     .features({"income", "region", "balance"})
                                     .note(lymph::DataNotePolicy::Factorize);
  const lymph::FeatureBatch batch = payload.feature_batch();
  const Series transformed =
      lymph::resolve(transformer.learn_map(batch), "feature_transformer.learn_map");

  std::cout << "== feature_transformer ==\n";
  std::cout << "trained=" << transformer.trained()
            << " stage_count=" << transformer.stage_count()
            << " rows=" << transformed.sizes()[0]
            << " features=" << transformed.sizes()[1] << "\n";
  std::cout << transformed << "\n";

  return 0;
}
```

Output

```text
== feature_transformer ==
trained=1 stage_count=2 rows=8 features=7
pluss::Series(shape=[8,7], strides=[7,1], dtype=float64, platform=cpu, numel=56, contiguous=true)
data=[[-1.3608276, 0, ... , 0.2],
  [-1.0886621, 0, ... , 0.4],
  ...
 [1.3608276, 1, ... , 2.7]]
```

Read that output as evidence of three things:

- the transformer is trained state, not a stateless helper
- two routed stages were learned and applied
- the output width expanded because one branch was categorical and got encoded

## Related Pages

- [ColumnRouter](ColumnRouter.md)
- [Stage](Stage.md)
- [OneHotEncoder](OneHotEncoder.md)
