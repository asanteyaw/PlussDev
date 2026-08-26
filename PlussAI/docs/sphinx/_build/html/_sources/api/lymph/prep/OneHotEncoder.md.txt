# `lymph::prep::OneHotEncoder`

## Summary

`OneHotEncoder` expands categorical levels into indicator features.

Use it when a categorical feature should be represented explicitly rather than as
an ordinal code.

## Why It Matters

Many learners consume numeric feature spaces.

If you map categories like:

- `north -> 0`
- `south -> 1`
- `west -> 2`

and then hand those codes directly to a linear learner, the learner may interpret
those labels as ordered magnitudes. One-hot encoding removes that false ordering
by allocating one indicator feature per represented level.

## Option Surface

`OneHotEncoder` is configured through `lymph::prep::OneHotEncoderOptions`.

### `max_categories`

Controls the maximum explicit category budget.

Effects:

- larger values preserve more detail
- smaller values control feature explosion more aggressively

### `overflow`

Controls what happens when observed category count exceeds the configured cap.

Typical consequence:

- strict policies fail early
- pragmatic policies keep the most frequent levels
- aggressive policies may skip the feature entirely

### `include_unknown_bucket`

Controls whether unseen categories can land in an explicit unknown bucket.

Effects:

- `true` is usually more robust for inference
- `false` is stricter and assumes category space is tightly controlled

## Signature

```cpp
lymph::prep::OneHotEncoder lymph::prep::OneHotEncoder(
    lymph::prep::OneHotEncoderOptions options = {});
lymph::prep::OneHotEncoder& lymph::prep::OneHotEncoder::features(
    std::initializer_list<std::string_view> names);
pluss::table::DataFrame lymph::prep::OneHotEncoder::learn_map(
    const pluss::table::DataFrame& frame,
    const lymph::TrainOptions& learn_options = {},
    const lymph::InferOptions& infer_options = {});
```

## Practical Examples

This page uses the exact verifier program
`lymph_prep_one_hot_encoder_docverify_demo.cpp`.

### Example 1

This example fits a standalone encoder on one categorical feature and prints the
expanded indicator frame.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::prep::OneHotEncoder encoder(lymph::prep::OneHotEncoderOptions{
      .max_categories = 8,
      .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
  });
  encoder.features({"region"});
  const DataFrame encoded = encoder.learn_map(raw.features({"region"}));
  const lymph::prep::TransformerSummary summary = encoder.summary();
  const DataFrame inference({
      {"region", Thread(pluss::make_note({"north", "west"}))},
  });
  const DataFrame mapped = encoder.map(inference);

  std::cout << "== one_hot_encoder ==\n";
  std::cout << "rows=" << encoded.ncases()
            << " features=" << encoded.nfeatures()
            << " summary_features=" << summary.feature_names.size() << "\n";
  std::cout << encoded.head(3) << "\n";
  std::cout << "== one_hot_encoder_inference ==\n";
  std::cout << "rows=" << mapped.ncases()
            << " features=" << mapped.nfeatures() << "\n";
  std::cout << mapped << "\n";

  return 0;
}
```

Output

```text
== one_hot_encoder ==
rows=8 features=5 summary_features=1
=======================================================
     | region_0  region_1  region_2  region_3  region_4
-------------------------------------------------------
0    | 0         0         0         1         0
1    | 0         0         1         0         0
2    | 0         0         0         1         0       
-------------------------------------------------------
[3 cs x 5 feat]
== one_hot_encoder_inference ==
rows=2 features=5
=======================================================
     | region_0  region_1  region_2  region_3  region_4
-------------------------------------------------------
0    | 0         0         0         1         0
1    | 0         0         1         0         0
-------------------------------------------------------
[2 cs x 5 feat]
```

The important thing is not just that feature count grew from one to five. It is
that the expansion is explicit, inspectable, and learned under a policy surface
that governs category budget and unknown handling.

### Example 2

The second half of the same executed program maps a later frame after
construction and fitting. It reuses the learned category layout, so downstream
data receives the same indicator columns as the training data.

```cpp
const DataFrame inference({
    {"region", Thread(pluss::make_note({"north", "west"}))},
});
const DataFrame mapped = encoder.map(inference);

std::cout << "== one_hot_encoder_inference ==\n";
std::cout << "rows=" << mapped.ncases()
          << " features=" << mapped.nfeatures() << "\n";
std::cout << mapped << "\n";
```

Output

```text
== one_hot_encoder_inference ==
rows=2 features=5
=======================================================
     | region_0  region_1  region_2  region_3  region_4
-------------------------------------------------------
0    | 0         0         0         1         0
1    | 0         0         1         0         0
-------------------------------------------------------
[2 cs x 5 feat]
```

## Related Pages

- [Stage](Stage.md)
- [StandardScaler](StandardScaler.md)
- [FeatureTransformer](FeatureTransformer.md)
