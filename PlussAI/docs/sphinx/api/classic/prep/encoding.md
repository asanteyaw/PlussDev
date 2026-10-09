# `pluss::ai::classic::prep::encoding`

`encoding` owns turning categorical values into numeric columns a model can
consume. Every encoder here fits a vocabulary (or, for
[`HashingEncoder`](encoding/HashingEncoder.md), a fixed hash range) from the
training batch during `learn`, then applies that fitted mapping during `map`
— the same `TransformerBase` shape [`scaling`](scaling.md) uses, applied to
categories instead of magnitudes.

`encoding` does not decide *which* columns of a `DataFrame` are categorical
in the first place (see [`DataFrameBatchOptions`](../data/DataFrameBatchOptions.md)'s
`categorical_policy` for the coarser, automatic path); it is for the case
where you want a specific encoding strategy, its own tunable options, and a
fitted vocabulary that travels with the model.

## The Surface

| Type | Output width per feature | Needs a target | Reach for it when |
| --- | --- | --- | --- |
| [`OneHotEncoder`](encoding/OneHotEncoder.md) | one column per category (+1 for unknown) | no | low-cardinality categories, no ordinal relationship implied |
| [`FrequencyEncoder`](encoding/FrequencyEncoder.md) | 1 | no | many categories, a rough popularity signal is enough |
| [`OrdinalEncoder`](encoding/OrdinalEncoder.md) | 1 | no | categories that already have a natural order |
| [`LabelEncoder`](encoding/LabelEncoder.md) | 1 | no | encoding a **target** column (classification labels), not a feature |
| [`BaseNEncoder`](encoding/BaseNEncoder.md) | `ceil(log_base(K+1))` | no | high cardinality, want fewer columns than one-hot without hashing |
| [`HashingEncoder`](encoding/HashingEncoder.md) | `n_components` (fixed, chosen up front) | no | very high or unbounded cardinality; no vocabulary to store or grow |
| [`TargetEncoder`](encoding/TargetEncoder.md) | 1 | **yes** | a strong per-category numeric signal is worth the leakage risk |
| [`WOEEncoder`](encoding/WOEEncoder.md) | 1 | **yes** (binary 0/1) | credit-scoring-style binary classification, log-odds scale |

## The Shared Shape, And Where It Diverges

Every encoder fits categories from the training batch and handles a category
`map` has never seen the same way in spirit — governed by
[`InferOptions::unknown_categorical`](../core/InferOptions.md#unknown_categorical)
(`Error` / `MissingBucket` / `Ignore`) — with two documented exceptions:
[`BaseNEncoder`](encoding/BaseNEncoder.md) and
[`HashingEncoder`](encoding/HashingEncoder.md) both encode entirely through
vectorized `Series` arithmetic and never consult `unknown_categorical` at
all; see each page's own `## Errors` section for exactly what an unseen
value produces there instead.

Every encoder also accepts a single `pluss::table::Slate` directly,
alongside the usual [`FeatureBatch`](../data/FeatureBatch.md) overload — a
convenience for the common case of encoding one raw table column without
building a batch around it by hand.

## How To Call It

Learn a one-hot encoding over one categorical column with three levels, then map it. The encoder adds one column for unknown values, so the program prints four columns.

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
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 0, 1, 2}, {6, 1}, cfg);
  cls::FeatureBatch batch{};
  batch.x = x;
  cls::prep::OneHotEncoderOptions options{};
  cls::prep::OneHotEncoder encoder(options);
  encoder.learn(batch);
  const pluss::Series encoded = encoder.map(batch).value();
  std::cout << "encoded columns = " << encoded.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
encoded columns = 4
```

[`TargetEncoder`](encoding/TargetEncoder.md) and
[`WOEEncoder`](encoding/WOEEncoder.md) alone need a target at fit time —
call `learn_supervised(batch, target)` instead of `learn(batch)`; see
[`encoding/Introduction`](encoding/Introduction.md) for why, and
[`TransformerBase::learn_supervised`](../estimators/TransformerBase.md) for
the shared interface method this overrides.

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`compose`](../compose.md) — `Flow::add_one_hot_encoder` and its seven
  siblings, routing an encoder into a larger pipeline.
- [`sparse`](../sparse.md) — a CSR-native one-hot path for when the encoded
  width itself is the memory concern, not just the category count.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

encoding/Introduction
encoding/OneHotEncoder
encoding/FrequencyEncoder
encoding/OrdinalEncoder
encoding/LabelEncoder
encoding/BaseNEncoder
encoding/HashingEncoder
encoding/TargetEncoder
encoding/WOEEncoder
```
<!-- apiref:toctree:end -->
