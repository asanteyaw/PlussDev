# `pluss::ai::classic::prep::imputation`

`imputation` owns filling in missing (`NaN`) values — the one sub-family in
`prep` whose entire purpose is to make `NaN` disappear rather than reshape a
value that is already present. Every imputer here fits per-feature fill
statistics (or, for [`IterativeImputer`](imputation/IterativeImputer.md), a
small predictive model) during `learn`, then replaces every `NaN` it sees
during `map` — a present value is always left untouched.

`imputation` does not decide whether a missing value is even acceptable in
the first place; that is [`TrainOptions`](../core/TrainOptions.md#missing) /
[`InferOptions`](../core/InferOptions.md#missing)'s `missing` field, checked
before an estimator's own `learn`/`infer` ever runs. Reach for an imputer
explicitly when you want to fill values yourself — as a `prep` stage you
control — rather than delegate the decision to that coarser, all-or-nothing
policy.

## The Surface

| Type | Fill source | Reach for it when |
| --- | --- | --- |
| [`SimpleImputer`](imputation/SimpleImputer.md) | one fixed statistic per feature (mean/median/most-frequent/constant) | a fast, per-feature-independent fill is enough |
| [`NeighborImputer`](imputation/NeighborImputer.md) | the value's *k* nearest rows, by the other features they share | rows with similar feature profiles are likely to have similar missing values too |
| [`IterativeImputer`](imputation/IterativeImputer.md) | a per-feature ridge regression against every other feature, refined over several rounds | features are correlated enough that predicting one from the others beats either fixed statistic above |

Every imputer's fitted fill is per-feature and independent of the others
(picking `Mean` for one feature does not require it for every feature) —
except `IterativeImputer`, whose whole premise is using every other
feature's values to predict the one being filled.

## How To Call It

Learn a median imputer on a column with a large outlier, then fill the batch. The program prints the row count.

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
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 100.0}, {6, 1}, cfg);
  cls::FeatureBatch batch{};
  batch.x = x;
  cls::prep::SimpleImputerOptions options{};
  options.strategy = cls::prep::SimpleImputerStrategy::Median;
  cls::prep::SimpleImputer imputer(options);
  imputer.learn(batch);
  const pluss::Series filled = imputer.map(batch).value();
  std::cout << "rows = " << filled.sizes()[0] << "\n";
  return 0;
}
```

Output

```text
rows = 6
```

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`core`](../core.md) — `TrainOptions` / `InferOptions`'s coarser `missing`
  policy, checked before an estimator's own `learn`/`infer` runs at all.
- [`compose`](../compose.md) — `Flow::add_simple_imputer` and its two
  siblings, routing an imputer into a larger pipeline.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

imputation/Introduction
imputation/SimpleImputer
imputation/NeighborImputer
imputation/IterativeImputer
```
<!-- apiref:toctree:end -->
