# `pluss::ai::classic::inspection`

`inspection` owns three free functions for understanding an
already-fitted model's behavior — each one builds its own model from a
[`ModelSpec`](families/ModelSpec.md) internally, then studies it:
[`permutation_impact`](inspection/permutation_impact.md) (held-out,
metric-based feature importance) and
[`dependence_curve`](inspection/dependence_curve.md)/
[`dependence_surface`](inspection/dependence_surface.md) (one- and
two-feature partial dependence). See
[`Introduction`](inspection/Introduction.md) for how importance and
dependence answer genuinely different questions, and why classification
support here is narrower (binary only) than everywhere else in this
reference.

## The Surface

| Function | What it gives you | Reach for it when |
| --- | --- | --- |
| [`permutation_impact`](inspection/permutation_impact.md) | per-feature score drop when that feature is shuffled on held-out data | you want to know whether a feature matters to the fitted model |
| [`dependence_curve`](inspection/dependence_curve.md) | mean prediction as one feature sweeps its own observed range | you want to see how the prediction responds to one feature |
| [`dependence_surface`](inspection/dependence_surface.md) | mean prediction over a 2-D grid of two features | the same, for two features swept jointly (interaction shape) |

## How To Call It

Score each feature by permutation impact and compute a dependence curve for feature 0. The program prints how many features were scored and how many points the curve has.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <iostream>
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
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};
  const cls::PermuteImpactReport importance = cls::permutation_impact(spec, batch, batch).value();
  std::cout << "features scored = " << importance.mean_drop.numel() << "\n";
  const cls::DependenceCurve curve =
      cls::dependence_curve(spec, batch, batch.features, /*feature_index=*/0).value();
  std::cout << "curve points = " << curve.grid.numel() << "\n";
  return 0;
}
```

Output

```text
features scored = 2
curve points = 32
```

## Related Pages

- [`families`](families.md) — [`ModelSpec`](families/ModelSpec.md), which
  every function here builds and fits internally.
- [`metrics`](metrics.md) — every `MetricKind`
  [`permutation_impact`](inspection/permutation_impact.md) can score
  against.
- [`features`](features.md) — a pre-training, label-correlation-only
  feature score, contrasted with this family's post-training one.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

inspection/Introduction
inspection/permutation_impact
inspection/dependence_curve
inspection/dependence_surface
```
<!-- apiref:toctree:end -->
