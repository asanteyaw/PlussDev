# `pluss::ai::classic::prep::ThresholdBinarizerOptions`

The threshold and the two output values consumed by
[`ThresholdBinarizer::map`](ThresholdBinarizer.md).

## Fields

### `threshold`

`double threshold = 0.0`

The cutoff. A value strictly greater than `threshold` maps to `high_value`;
everything else (including a value exactly equal to `threshold`) maps to
`low_value`. Must be finite.

### `low_value`

`double low_value = 0.0`

The output for a value `<= threshold`. Must be finite.

### `high_value`

`double high_value = 1.0`

The output for a value `> threshold`. Must be finite.

## Guidance

The default `threshold = 0`, `low_value = 0`, `high_value = 1` is the
common case for a feature already centered so `0` is the meaningful
boundary (often paired with a prior [`scaling`](../scaling.md) stage).
Adjust `low_value`/`high_value` to `-1`/`1` for a signed output some models
prefer over `0`/`1`.

## See Also

- [`ThresholdBinarizer`](ThresholdBinarizer.md) — the consuming type.
