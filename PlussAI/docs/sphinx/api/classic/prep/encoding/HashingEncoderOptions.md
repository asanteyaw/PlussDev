# `pluss::ai::classic::prep::HashingEncoderOptions`

The output width and hash seed consumed by
[`HashingEncoder::learn`](HashingEncoder.md) / `map`.

## Fields

### `n_components`

`std::int64_t n_components = 8`

The fixed number of output buckets (and output columns) per input feature,
chosen up front independent of cardinality. Must be `>= 1`; `learn` rejects
anything smaller with `Status::invalid_argument`. Larger values reduce
collision risk (two different values hashing to the same bucket) at the
cost of a wider output; there is no vocabulary to consult when choosing this
— it is a direct trade between output width and collision rate for however
many distinct values you expect the column to actually carry.

### `seed`

`std::uint64_t seed = 0`

Perturbs which bucket each value hashes into, without changing
`n_components` or the collision rate. Two `HashingEncoder`s constructed with
the same `n_components` and `seed` hash identically; changing `seed` alone
reshuffles the assignment. Set it explicitly only if you need a specific
column's hashing to differ from another column encoded with otherwise
identical options (rare — the default `0` is fine for ordinary use, since
each `HashingEncoder` instance you construct is already independent of every
other).

## Guidance

Pick `n_components` from the cardinality you expect and how much collision
risk is acceptable — there is no fitted vocabulary to inspect afterward, so
this choice cannot be corrected after the fact without re-fitting with a
different value. Leave `seed` at its default unless you have a specific
reason two encoders need to disagree on bucket assignment.

## See Also

- [`HashingEncoder`](HashingEncoder.md) — the consuming type.
- [`BaseNEncoderOptions`](BaseNEncoderOptions.md) — the bounded-vocabulary
  alternative's options.
