# `pluss::ai::classic::features`

`features` owns six free functions for reducing a feature matrix's column
count: [`take_features`](features/take_features.md) (plain index
selection), [`variance_prune`](features/variance_prune.md) (unsupervised,
drops near-constant columns), and four supervised, F-statistic-based
functions — [`rank_features`](features/rank_features.md)/
[`pick_features`](features/pick_features.md)/
[`f_score_rank`](features/f_score_rank.md)/
[`f_regression_rank`](features/f_regression_rank.md). Nothing here is a
persistent object — every function takes a batch and a plain options
struct, and returns a plain result struct; see
[`Introduction`](features/Introduction.md) for how the four scoring
functions relate to each other and two facts worth knowing before reaching
for any of them (`absolute_score` currently does nothing; `variance_prune`
and `pick_features` order their reduced batches differently).

## The Surface

| Function | What it gives you | Reach for it when |
| --- | --- | --- |
| [`take_features`](features/take_features.md) | an explicit column subset, any order, duplicates allowed | you already know exactly which columns you want |
| [`variance_prune`](features/variance_prune.md) | near-constant columns dropped, no labels needed | a quick unsupervised cleanup pass |
| [`rank_features`](features/rank_features.md) | every feature's F-score, plus a full ranking | you want the complete picture, not just a cut |
| [`pick_features`](features/pick_features.md) | a reduced `FeatureBatch`, top-k or top-fraction | selecting the best features by a labeled target |
| [`f_score_rank`](features/f_score_rank.md) | just the top-k indices, classification F-score | a minimal classification feature-selection call |
| [`f_regression_rank`](features/f_regression_rank.md) | just the top-k indices, regression F-score | a minimal regression feature-selection call |

## How To Call It

Keep only the strongest feature column with `pick_features`. The program prints how many columns the reduced batch keeps.

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
  cls::LabeledBatch batch = fixture_batch(cfg);
  cls::FeaturePickOptions options{};
  options.top_k = 1;
  const cls::FeaturePickResult picked = cls::pick_features(batch, options).value();
  std::cout << "kept columns = " << picked.reduced.x.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
kept columns = 1
```

## Related Pages

- [`data`](data.md) — [`FeatureBatch`](data/FeatureBatch.md)/
  [`LabeledBatch`](data/LabeledBatch.md), every function's input type.
- [`prep`](prep.md) — column transforms (scaling, encoding) that
  ordinarily run before, not after, feature selection.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

features/Introduction
features/take_features
features/variance_prune
features/rank_features
features/pick_features
features/f_score_rank
features/f_regression_rank
```
<!-- apiref:toctree:end -->
