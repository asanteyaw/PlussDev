# `pluss::ai::classic::prep::FunctionTransformerOptions`

The input-validation and sample-count guard toggles consumed by
[`FunctionTransformer::learn`](FunctionTransformer.md) / `map`.

## Fields

### `validate_input`

`bool validate_input = true`

Whether `learn` and `map` run [`validate_features`](../../data/FeatureBatch.md)
against the batch before doing anything else. `true` (the default) catches
an undefined or rank-1 batch with the same clear error every other `prep`
kernel gives; set `false` only if your callable is meant to accept input
shapes `validate_features` would otherwise reject.

### `preserve_sample_count`

`bool preserve_sample_count = true`

Whether `map` requires the callable's output to have the same sample count
as its input. `true` (the default) catches a callable that accidentally
drops or duplicates rows — see
[`FunctionTransformer`'s Example 2](FunctionTransformer.md#example-2). Set
`false` only if the callable is deliberately meant to change row count
(rare for a `prep` stage, since every downstream consumer generally expects
one output row per input row).

## Guidance

Leave both fields at their default `true` unless you have a specific,
deliberate reason the callable needs to bypass one of these guards — they
exist to catch the same class of mistake a hand-written callable is more
prone to than a named `prep` kernel.

## See Also

- [`FunctionTransformer`](FunctionTransformer.md) — the consuming type.
