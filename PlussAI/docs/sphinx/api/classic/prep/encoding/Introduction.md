# `classic::prep::encoding` Introduction

## The Shared Contract

Every encoder in this sub-family fits a **vocabulary** — a fixed list of
category values seen during `learn` — except
[`HashingEncoder`](HashingEncoder.md), which fits nothing at all beyond the
feature count (its output bucket is a pure function of the value and the
configured `seed`/`n_components`, computed fresh on every `map`). Six of the
eight share the same vocabulary-fitting core: count each category's
occurrences, drop or cap it against `max_categories` per
[`CategoryOverflowPolicy`](OneHotEncoderOptions.md), then sort the surviving
keys for a deterministic column order.

Two structurally different fit paths exist:

- **Unsupervised** (`OneHotEncoder`, `FrequencyEncoder`, `OrdinalEncoder`,
  `LabelEncoder`, `BaseNEncoder`, `HashingEncoder`): call `learn(batch)` —
  the ordinary `TransformerBase` path, no target involved.
- **Supervised** (`TargetEncoder`, `WOEEncoder`): call
  `learn_supervised(batch, target)` instead — each category's *fitted value*
  is itself computed from the target, not just the category's presence.
  Calling plain `learn(batch)` on either returns `Status::invalid_argument`
  (`needs_target()` is `true` for both); this is what lets
  [`compose::Flow::learn`](../../compose/Flow/learn.md) recognize them and
  route the label through automatically for a stage built inside a `Flow`.

## Unseen Categories At `map` Time

For every encoder except `BaseNEncoder` and `HashingEncoder`, a category
`map` sees that was not present at `learn` time (or was dropped by
`max_categories`) is governed by
[`InferOptions::unknown_categorical`](../../core/InferOptions.md#unknown_categorical):

- `Error` (the default) — `map` fails with `Status::invalid_argument`.
- `MissingBucket` — routed to a reserved "unknown" output (a dedicated
  one-hot column, a configured fallback frequency, an extra ordinal index,
  or — for the two supervised encoders — the training target's own prior).
- `Ignore` — folded into whatever the encoder's ordinary zero/`NaN` output
  is, with no distinct signal that the value was unrecognized.

`BaseNEncoder` and `HashingEncoder` never consult `unknown_categorical` —
both encode through vectorized `Series` arithmetic with no per-value branch
at all, so an unseen value falls out of the same formula every other value
does rather than being special-cased. See each page's `## Errors` section.

## A Missing Value Is Not An Unknown Category

`NaN` is excluded from the fitted vocabulary and, at `map` time, is handled
by `NaN`-in/`NaN`-out or a policy-driven fallback depending on the encoder —
never conflated with a genuinely unseen category. Pair a categorical column
that has real missing values with [`imputation`](../imputation.md) first if
you need every row to carry a concrete category.

## Choosing Among The Eight

- Low cardinality, no implied order: [`OneHotEncoder`](OneHotEncoder.md).
- High cardinality, want to keep the column count down without hashing:
  [`BaseNEncoder`](BaseNEncoder.md) (bounded, deterministic vocabulary) or
  [`HashingEncoder`](HashingEncoder.md) (unbounded cardinality, fixed width,
  no vocabulary to store).
- A natural order already exists: [`OrdinalEncoder`](OrdinalEncoder.md).
- Encoding the **target** column itself for a classifier:
  [`LabelEncoder`](LabelEncoder.md) — distinct from `OrdinalEncoder` mainly
  in its default (`one_based = false`), matching a 0-indexed class-label
  convention.
- A rough popularity signal, cheaply: [`FrequencyEncoder`](FrequencyEncoder.md).
- Willing to accept target-leakage risk for a strong single-column numeric
  signal: [`TargetEncoder`](TargetEncoder.md) (any target) or
  [`WOEEncoder`](WOEEncoder.md) (binary target, log-odds scale, the
  credit-scoring convention).

## Reading Order

1. [`OneHotEncoder`](OneHotEncoder.md) — read this one in full; every other
   unsupervised encoder in this family reuses the same fit-a-vocabulary
   shape with a different output formula.
2. [`OrdinalEncoder`](OrdinalEncoder.md) / [`LabelEncoder`](LabelEncoder.md) —
   near-identical to each other, differing only in the `one_based` default.
3. [`FrequencyEncoder`](FrequencyEncoder.md), [`BaseNEncoder`](BaseNEncoder.md),
   [`HashingEncoder`](HashingEncoder.md) — reached for by cardinality.
4. [`TargetEncoder`](TargetEncoder.md) / [`WOEEncoder`](WOEEncoder.md) — the
   two supervised encoders, read together since they share
   `learn_supervised` and differ only in the fitted formula.

## Related Pages

- [`prep`](../../prep.md) — the parent family.
- [`compose`](../../compose.md) — `Flow::add_one_hot_encoder` and its seven
  siblings; `Flow::learn` is what actually supplies a target to a supervised
  encoder stage.
- [`estimators`](../../estimators.md) — `TransformerBase`, including
  `needs_target()` / `learn_supervised`.
