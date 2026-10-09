# `pluss::ai::classic::prep::transform`

`transform` owns everything in `prep` that does not fit `scaling`,
`encoding`, or `imputation`: reshaping a distribution's *shape* rather than
just its magnitude ([`PowerMap`](transform/PowerMap.md),
[`QuantileMap`](transform/QuantileMap.md)), normalizing per-row rather than
per-feature ([`VectorNormalizer`](transform/VectorNormalizer.md)),
thresholding or expanding features
([`ThresholdBinarizer`](transform/ThresholdBinarizer.md),
[`PolynomialLift`](transform/PolynomialLift.md)), and routing custom or
composite logic of your own
([`FunctionTransformer`](transform/FunctionTransformer.md),
[`FeatureTransformer`](transform/FeatureTransformer.md)).

## The Surface

| Type | What it does | Reach for it when |
| --- | --- | --- |
| [`VectorNormalizer`](transform/VectorNormalizer.md) | scales each **row** to unit L1/L2/L∞ norm | row magnitude, not any one feature's scale, should be normalized (text/embedding vectors) |
| [`QuantileMap`](transform/QuantileMap.md) | maps each value to its fitted empirical percentile (optionally through the normal quantile function) | a distribution's actual shape (not just its scale) should be flattened to uniform or Gaussian |
| [`PowerMap`](transform/PowerMap.md) | fits a Yeo-Johnson or Box-Cox power transform per feature | a skewed distribution should be made more symmetric before a model that assumes roughly-normal input |
| [`ThresholdBinarizer`](transform/ThresholdBinarizer.md) | maps every value to one of two fixed outputs by a threshold | a continuous feature should become a hard yes/no signal |
| [`PolynomialLift`](transform/PolynomialLift.md) | expands features into their polynomial terms up to a degree | a linear model needs explicit interaction or curvature terms |
| [`FunctionTransformer`](transform/FunctionTransformer.md) | wraps an arbitrary `Series -> Series` callable as a stage | none of the above cover a one-off transform you already have code for |
| [`FeatureTransformer`](transform/FeatureTransformer.md) | its own routed sub-pipeline of other transformers | several transforms need to be composed and routed *before* being handed to a `compose::Flow` as a single stage |

## How To Call It

Fit a Yeo-Johnson power map on one column, then transform it. The program prints the value count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  cls::FeatureBatch train{};
  train.x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 8.0}, {5, 1}, cfg);
  cls::prep::PowerMapOptions options{};
  options.mode = cls::prep::PowerMapMode::YeoJohnson;
  cls::prep::PowerMap power_map(options);
  power_map.learn(train);
  const pluss::Series reshaped = power_map.map(train).value();
  std::cout << "values = " << reshaped.numel() << "\n";
  return 0;
}
```

Output

```text
values = 5
```

[`FunctionTransformer`](transform/FunctionTransformer.md) and
[`FeatureTransformer`](transform/FeatureTransformer.md) are constructed
differently from every other `prep` kernel — the former takes a callable
instead of an options struct, the latter is built up with its own
`add_stage`/`add_pipeline` calls rather than fit directly — see their own
pages.

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`compose`](../compose.md) — `Flow::add_vector_normalizer` and its five
  named siblings, plus `add_function_transformer` / `add_feature_transformer`,
  routing any of these into a larger pipeline. `Flow`'s own stage
  routing/remainder mechanics are the same ones
  [`FeatureTransformer`](transform/FeatureTransformer.md) uses internally.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

transform/Introduction
transform/VectorNormalizer
transform/QuantileMap
transform/PowerMap
transform/ThresholdBinarizer
transform/PolynomialLift
transform/FunctionTransformer
transform/FeatureTransformer
```
<!-- apiref:toctree:end -->
