# `classic::prep::transform` Introduction

## What This Family Owns

`transform` is the catch-all sub-family for everything in `prep` that
reshapes numeric feature values without being a scaler, a categorical
encoder, or an imputer. Five of its seven types
([`VectorNormalizer`](VectorNormalizer.md), [`QuantileMap`](QuantileMap.md),
[`PowerMap`](PowerMap.md), [`ThresholdBinarizer`](ThresholdBinarizer.md),
[`PolynomialLift`](PolynomialLift.md)) are ordinary
[`TransformerBase`](../../estimators/TransformerBase.md) kernels with their
own options struct, fit the usual way. The remaining two
([`FunctionTransformer`](FunctionTransformer.md),
[`FeatureTransformer`](FeatureTransformer.md)) are structurally different —
one wraps your own callable, the other is a routed sub-pipeline of other
transformers — and are read separately.

## Per-Feature Versus Per-Row

Every other kernel in `prep` (every scaler, every encoder, every imputer)
computes its statistics **down each feature column**. `VectorNormalizer` is
the one exception in this family, and the whole reason it exists
separately from `scaling`: it normalizes **across the features of each
row**, an entirely different axis. Keep this distinction in mind reaching
for it — "normalize this row's magnitude" (`VectorNormalizer`) is a
different operation from "normalize this feature's magnitude across rows"
([`scaling`](../scaling.md)), even though both use the word "normalize."

## Two Ways To Bring Your Own Logic

- [`FunctionTransformer`](FunctionTransformer.md) wraps a single
  `Series -> Series` callable you already have as a `TransformerBase` stage
  — the escape hatch for a one-off transform none of the named kernels
  cover.
- [`FeatureTransformer`](FeatureTransformer.md) is its own composable
  sub-pipeline: `add_stage`/`add_pipeline` calls route other transformers
  (any `TransformerBase`, including a `FunctionTransformer`) to feature
  subsets with a remainder policy, fit and applied together as one unit —
  structurally identical to how [`compose::Flow`](../../compose/Flow.md)
  routes its own stages, just usable standalone or nested as a single
  `Flow` stage itself.

## Reading Order

1. [`PowerMap`](PowerMap.md) / [`QuantileMap`](QuantileMap.md) — the two
   distribution-shape transforms, read together since they solve
   overlapping problems differently.
2. [`VectorNormalizer`](VectorNormalizer.md),
   [`ThresholdBinarizer`](ThresholdBinarizer.md),
   [`PolynomialLift`](PolynomialLift.md) — reached for by name.
3. [`FunctionTransformer`](FunctionTransformer.md) /
   [`FeatureTransformer`](FeatureTransformer.md) — read when a project needs
   custom or composed logic beyond the five named kernels.

## Related Pages

- [`prep`](../../prep.md) — the parent family.
- [`scaling`](../scaling.md) — the per-feature counterpart to
  `VectorNormalizer`'s per-row normalization.
- [`compose`](../../compose.md) — `Flow`'s own stage routing, the same
  mechanics `FeatureTransformer` uses internally.
