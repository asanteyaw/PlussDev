# `pluss::ai::classic::prep::LabelEncoderOptions`

The vocabulary cap, overflow policy, unknown-bucket toggle, and index base
consumed by [`LabelEncoder::learn`](LabelEncoder.md) / `map` — field-for-field
identical to [`OrdinalEncoderOptions`](OrdinalEncoderOptions.md), with one
different default.

## Fields

### `max_categories`

`std::int64_t max_categories = 256`

As [`OrdinalEncoderOptions::max_categories`](OrdinalEncoderOptions.md#max_categories).

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow).

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

As [`OrdinalEncoderOptions::include_unknown_bucket`](OrdinalEncoderOptions.md#include_unknown_bucket).

### `one_based`

`bool one_based = false`

**Defaults `false`** here, unlike `OrdinalEncoderOptions` (`true`) — the
0-indexed convention a classification target's fitted class labels are
expected to follow through `classic`. Set `true` only if a specific
downstream consumer genuinely needs a 1-indexed label.

## Guidance

Leave every field at its default unless encoding a target with a
cardinality above 256 distinct classes (raise `max_categories`) or a
specific reason to depart from 0-indexed labels (`one_based = true`).

## See Also

- [`LabelEncoder`](LabelEncoder.md) — the consuming type.
- [`OrdinalEncoderOptions`](OrdinalEncoderOptions.md) — the identical
  fields, `one_based` defaulting `true`, meant for a feature column.
