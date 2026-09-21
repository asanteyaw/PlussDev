# `pluss::ai::classic::prep::FrequencyEncoderOptions`

The vocabulary cap, overflow policy, and unknown-value fallback consumed by
[`FrequencyEncoder::learn`](FrequencyEncoder.md) / `map`.

## Fields

### `max_categories`

`std::int64_t max_categories = 50`

The largest vocabulary `learn` will fit per feature before `overflow`
applies. `0` means unlimited. Must be `>= 0`. A higher default than
[`OneHotEncoderOptions::max_categories`](OneHotEncoderOptions.md#max_categories)
since a dropped category here only costs a fallback frequency, not an
entire output column.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow) —
the same shared three-value policy, documented in full there.

### `unknown_frequency`

`double unknown_frequency = 0.0`

The value `map` assigns a category it has never seen, when
`InferOptions::unknown_categorical = MissingBucket`. Must be in `[0, 1]`.
The default `0.0` makes an unseen category read identically to a fitted
category with zero occurrences (and identically to `unknown_categorical =
Ignore`); raise it toward `1.0` to instead treat an unseen category as
*maximally* common, if that better matches your downstream model's
assumptions.

## Guidance

Leave `unknown_frequency` at `0.0` unless you have a specific reason an
unseen category should read as common rather than rare — `0.0` is the
conservative choice for most models, since a genuinely novel category is
usually closer to "rare" than "common" in practice.

## See Also

- [`FrequencyEncoder`](FrequencyEncoder.md) — the consuming type.
- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the full
  `CategoryOverflowPolicy` documentation.
- [`encoding/Introduction`](Introduction.md) — unseen-category handling at
  `map` time.
