# `lymph::options::HashingEncoder`

`options::HashingEncoder` (family alias `prep::HashingEncoderOptions`) configures
feature hashing. It is passed to the
[`prep::HashingEncoder`](HashingEncoder.md) constructor and to the
[`prep::stage::hashing_encoder`](../stage/hashing_encoder.md) factory.

### `HashingEncoderOptions` fields

| Field | Type | Default |
|---|---|---|
| `n_components` | `std::int64_t` | `8` |
| `seed` | `std::uint64_t` | `0` |

## Fields

### `n_components`

`std::int64_t n_components = 8`

The number of output (bucket) columns per encoded column, and the modulus of the
hash. More buckets means a wider frame and a lower chance that two distinct
categories collide into the same bucket. Values below `1` are rejected at fit
time.

### `seed`

`std::uint64_t seed = 0`

Perturbs the hash. The same categories map to a different bucket assignment for
a different `seed`; the assignment is otherwise fully determined by the value and
`n_components`, so a fixed `seed` gives reproducible encodings across runs and
frames.

## Related Pages

- [`prep::HashingEncoder`](HashingEncoder.md) — the object these options
  configure.
- [`prep::stage::hashing_encoder`](../stage/hashing_encoder.md) — the stage twin.
