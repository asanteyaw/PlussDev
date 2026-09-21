# `classic::prep` Introduction

## What This Family Owns

`prep` owns the concrete transform kernels — scalers, encoders, imputers,
distribution reshapers, and an outlier scorer — that every
[`compose::Flow`](../compose/Flow.md) stage is ultimately built from. Each one
is a small, independently usable class: construct it with an options struct,
`learn` it on a [`FeatureBatch`](../data/FeatureBatch.md), then `map` new
batches through the fitted transform. Nothing in `prep` knows about routing
columns to different stages, persisting more than its own state, or
combining several kernels into one pipeline — that is
[`compose`](../compose.md)'s job.

## The Shared Contract

Every class in `prep` implements [`TransformerBase`](../estimators/TransformerBase.md):

```cpp
bool trained() const noexcept;
classic::Status learn(const classic::FeatureBatch&, const classic::TrainOptions& = {});
classic::StatusOr<pluss::Series> map(const classic::FeatureBatch&, const classic::InferOptions& = {}) const;
classic::StatusOr<pluss::Series> learn_map(const classic::FeatureBatch&,
                                           const classic::TrainOptions& = {},
                                           const classic::InferOptions& = {});
classic::StatusOr<std::vector<pluss::Series>> dump_state() const;
classic::Status load_state(/* a range of pluss::Series */);
```

Three behaviors hold for every kernel here, so individual pages state them
once and do not repeat them per class:

- **`map` before `learn` fails.** Every kernel returns
  `Status::precondition` with a message naming the class
  (`"StandardScaler requires learn() before map()."`) rather than crashing or
  silently producing garbage — see [`TransformerBase`](../estimators/TransformerBase.md)
  for the pattern.
- **A missing value (`NaN`) passes straight through `map`, untouched**, for
  every numeric kernel in `scaling`, `imputation` (until filled),
  `transform`, and `outlier`. Statistics gathered during `learn` — a mean, a
  quantile, a category count — always exclude `NaN` entries rather than
  propagating them into the fit.
- **`dump_state()` / `load_state(state)` round-trip the fitted parameters as a
  `std::vector<pluss::Series>`**, the same pattern documented on
  [`EstimatorBase::dump_state`](../estimators/EstimatorBase/dump_state.md) /
  [`load_state`](../estimators/EstimatorBase/load_state.md) — the first
  element is always metadata (`trained`, `layout`, `feature_count`), the rest
  are the class's own fitted arrays in a fixed order.

## Choosing A Sub-family

- Numeric columns whose *magnitude* needs rescaling (so no single feature
  dominates a distance- or gradient-based model): [`scaling`](../prep/scaling.md).
- Categorical columns that need to become numeric at all: [`encoding`](../prep/encoding.md).
- Columns with missing values that a downstream model cannot tolerate:
  [`imputation`](../prep/imputation.md).
- Numeric columns whose *distribution shape* needs to change (skew, per-row
  norm, a threshold, a polynomial expansion) or that need custom or routed
  logic: [`transform`](../prep/transform.md).
- Flagging anomalous rows rather than reshaping columns: [`outlier`](../prep/outlier.md).

## Reading Order

1. [`scaling`](../prep/scaling.md) and [`encoding`](../prep/encoding.md) —
   the two sub-families almost every tabular pipeline touches.
2. [`imputation`](../prep/imputation.md) — only needed when the data actually
   has missing values.
3. [`transform`](../prep/transform.md) and [`outlier`](../prep/outlier.md) —
   reached for by name when a project needs them.

## Related Pages

- [`compose`](../compose.md) — routing several `prep` kernels, plus a model,
  into one `Flow`.
- [`estimators`](../estimators.md) — `TransformerBase`, the interface every
  kernel here implements.
- [`sparse`](../sparse.md) — sparse-native transforms for high-cardinality
  encoded output that a dense `FeatureBatch` would waste memory on.
