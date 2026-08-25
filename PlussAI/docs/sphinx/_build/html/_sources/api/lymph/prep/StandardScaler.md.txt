# `lymph::prep::StandardScaler`

## Summary

`StandardScaler` centers and scales numeric features using learned training-set
statistics.

Use it when numeric magnitudes should be brought onto a comparable scale before
learning.

## Why It Matters

Many learners are sensitive to raw numeric scale:

- linear models
- margin-based models
- distance-based models
- regularized optimization in general

Without scaling, one feature can dominate simply because it is measured in much
larger units than the rest.

## Option Surface

`StandardScaler` is configured through
`lymph::prep::StandardScalerOptions`.

### `with_centering`

If `true`, the learned mean is removed from each feature.

Effects:

- recenters the feature around zero
- helps optimization behave more predictably
- changes the transformed interpretation from "raw units" to "deviation from
  learned center"

### `with_scaling`

If `true`, each centered feature is divided by a learned scale.

Effects:

- reduces unit-driven magnitude disparities
- makes coefficients and gradients more comparable
- helps regularization behave more evenly across numeric features

### `variance_floor`

This is the numerical safety guard for near-constant features.

Effects:

- prevents unstable division when a feature has almost no variance
- trades literal scaling purity for numerical robustness when necessary

## Signature

```cpp
lymph::prep::StandardScaler lymph::prep::StandardScaler(
    lymph::prep::StandardScalerOptions options = {});
lymph::prep::StandardScaler& lymph::prep::StandardScaler::features(
    std::initializer_list<std::string_view> names);
pluss::table::DataFrame lymph::prep::StandardScaler::learn_map(
    const pluss::table::DataFrame& frame,
    const lymph::TrainOptions& learn_options = {},
    const lymph::InferOptions& infer_options = {});
```

## Practical Example

This page uses the exact verifier program
`lymph_prep_standard_scaler_docverify_demo.cpp`.

### Example 1

This example trains a standalone scaler on three numeric columns and prints both
the transformed frame and the learned summary width.

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::prep::StandardScaler scaler(lymph::prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .variance_floor = 1e-12,
  });

  const DataFrame scaled =
      scaler.learn_map(raw.features({"income", "balance", "late"}));
  const lymph::prep::StandardScalerSummary summary = scaler.summary();

  std::cout << "== standard_scaler ==\n";
  std::cout << "rows=" << scaled.ncases()
            << " features=" << scaled.nfeatures()
            << " summary_features=" << summary.feature_names.size() << "\n";
  std::cout << scaled.head(2) << "\n";

  return 0;
}
```

Output

```text
== standard_scaler ==
rows=8 features=3 summary_features=3
=======================================
     | income        balance       late
---------------------------------------
0    | -1.360827635  -1.300637865  -1  
1    | -1.088662108  -1.081583067  -1  
---------------------------------------
[2 cs x 3 feat]
```

Two things matter in that output:

- the transformed frame still has the same row count and feature count
- the values are now standardized numeric quantities rather than raw magnitudes

That preservation of row alignment is why `StandardScaler` works naturally as a
stage inside a larger model pipeline.

## Related Pages

- [Stage](Stage.md)
- [OneHotEncoder](OneHotEncoder.md)
- [FeatureTransformer](FeatureTransformer.md)
