# `lymph::options::LabelEncoder`

`options::LabelEncoder` (family alias `prep::LabelEncoderOptions`) configures
dense integer coding. It is passed to the
[`prep::LabelEncoder`](LabelEncoder.md) constructor and to the
[`prep::stage::label_encoder`](../stage/label_encoder.md) factory. It is
identical to [`options::OrdinalEncoder`](OrdinalEncoderOptions.md) except that
`one_based` defaults to `false`.

### `LabelEncoderOptions` fields

| Field | Type | Default |
|---|---|---|
| `max_categories` | `std::int64_t` | `256` |
| `overflow` | `CategoryOverflowPolicy` | `CategoryOverflowPolicy::KeepMostFrequent` |
| `include_unknown_bucket` | `bool` | `true` |
| `one_based` | `bool` | `false` |

## Fields

### `max_categories`

`std::int64_t max_categories = 256`

Cap on distinct categories tracked per column. Past this count, `overflow`
applies.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

`KeepMostFrequent` keeps the top `max_categories` and folds the rest into the
unseen slot; `Error` rejects the column; `SkipFeature` drops it.

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

Reserve a code for categories not seen at fit time; the replay-time behaviour is
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map).

### `one_based`

`bool one_based = false`

Codes start at `0`, giving a dense `0..K-1` range. This is the only default that
differs from [`options::OrdinalEncoder`](OrdinalEncoderOptions.md).

## Related Pages

- [`prep::LabelEncoder`](LabelEncoder.md) — the object these options configure.
- [`prep::stage::label_encoder`](../stage/label_encoder.md) — the stage twin.
