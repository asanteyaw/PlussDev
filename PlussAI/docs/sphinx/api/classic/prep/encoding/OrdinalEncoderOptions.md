# `pluss::ai::classic::prep::OrdinalEncoderOptions`

The vocabulary cap, overflow policy, unknown-bucket toggle, and index base
consumed by [`OrdinalEncoder::learn`](OrdinalEncoder.md) / `map`.

## Fields

### `max_categories`

`std::int64_t max_categories = 256`

The largest vocabulary `learn` will fit per feature before `overflow`
applies. `0` means unlimited. Must be `>= 0`. Higher than
[`OneHotEncoderOptions`](OneHotEncoderOptions.md#max_categories)'s default
since — like `FrequencyEncoder` — a dropped category here costs only a
fallback rank, not an output column.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow).

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

Whether an unseen category under `unknown_categorical = MissingBucket` gets
a reserved rank one past the last fitted category (`true`, the default) or
falls back to `NaN`, the same output `Ignore` always produces (`false`).

### `one_based`

`bool one_based = true`

Whether the smallest fitted category ranks `1` (`true`, the default) or `0`
(`false`). Purely a display/downstream-convention choice — the relative
ordering between categories is identical either way.

## Guidance

Leave `max_categories` generous (or unlimited) for a genuinely ordered
column, since dropping a middling category from an ordinal scale is more
disruptive to the ordering than dropping a rare category from a one-hot
vocabulary would be. `one_based` matters only if a downstream step
special-cases rank `0`; otherwise either convention works equally well.

## See Also

- [`OrdinalEncoder`](OrdinalEncoder.md) — the consuming type.
- [`LabelEncoderOptions`](LabelEncoderOptions.md) — the same fields, with
  `one_based` defaulting `false`.
- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the full
  `CategoryOverflowPolicy` documentation.
