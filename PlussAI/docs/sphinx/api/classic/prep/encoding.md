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

Every encoder also accepts a single `pluss::table::Thread` directly,
alongside the usual [`FeatureBatch`](../data/FeatureBatch.md) overload — a
convenience for the common case of encoding one raw table column without
building a batch around it by hand.

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::prep::OneHotEncoderOptions options{};
cls::prep::OneHotEncoder encoder(options);

encoder.learn(train_batch);
const pluss::Series encoded = encoder.map(train_batch).value();
// encoder.learn_map(train_batch) fuses the two calls above into one.
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
