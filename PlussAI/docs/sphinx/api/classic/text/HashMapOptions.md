# `pluss::ai::classic::HashMapOptions`

Configures [`HashMapper`](HashMapper.md)'s feature-hashing vectorization —
tokenization is shared conceptually with
[`LexiconMapOptions`](LexiconMapOptions.md) (`lowercase`/`min_ngram`/
`max_ngram` mean exactly the same thing), but there is no vocabulary to
cap or filter: every field here is instead read fresh on **every**
`map_dense`/`map_sparse` call, not validated once up front — see
[`HashMapper`'s Construction section](HashMapper.md#construction).

## Fields

### `lowercase`

`bool lowercase = true`

Whether every document is ASCII-lowercased before tokenization. Identical
meaning to
[`LexiconMapOptions::lowercase`](LexiconMapOptions.md#lowercase).

### `n_features`

`std::int64_t n_features = 32768`

The fixed output width — every hashed term lands in column `hash(term) %
n_features`, so the output shape never depends on how many distinct terms
were actually seen (unlike
[`LexiconMapper`](LexiconMapper.md), which grows a real vocabulary). A
smaller `n_features` increases the chance that two different terms hash to
the same column (a collision), silently combining their contributions —
see [`HashMapper`'s Example 3](HashMapper.md#example-3) for a real,
deliberately-forced one. Must be `> 0`.

### `binary`

`bool binary = false`

`false` (the default): a column accumulates every hashed term's own
**signed** contribution (see `alternate_sign` below) — repeats add up.
`true`: a column is set to that term's sign (`1.0`, or `-1.0` when
`alternate_sign` flips it) on any occurrence, however many times it
repeats. This is a genuinely different rule from
[`LexiconMapOptions::binary`](LexiconMapOptions.md#binary), which always
sets `1.0` regardless of sign — with `alternate_sign = true` here, a
`binary` column can still read `-1.0`, never just `0`/`1`. See
[`HashMapper`'s Example 5](HashMapper.md#example-5).

### `alternate_sign`

`bool alternate_sign = false`

`false` (the default): every hashed term contributes positively. `true`:
each term's hash's own top bit also decides its sign (`-1.0` when set,
`1.0` otherwise) before it is added into its column — the standard
hashing-trick mitigation that keeps collisions from systematically
inflating a column's magnitude, since two colliding terms are now about as
likely to cancel as to reinforce. See
[`HashMapper`'s Example 4](HashMapper.md#example-4).

### `min_ngram`

`std::int64_t min_ngram = 1`

The shorter end of the inclusive n-gram-length range, identical meaning
and per-document capping behavior to
[`LexiconMapOptions::min_ngram`](LexiconMapOptions.md#min_ngram) — see
`max_ngram` below for the longer end. Must be `> 0` and `<= max_ngram` —
checked on **every** `map_dense`/`map_sparse` call, not once at
construction. See [`HashMapper`'s Example 6](HashMapper.md#example-6).

### `max_ngram`

`std::int64_t max_ngram = 1`

The longer end of the `min_ngram`/`max_ngram` range above, identical
meaning to [`LexiconMapOptions::max_ngram`](LexiconMapOptions.md#max_ngram).
Must be `> 0`. Checked on every call, same as `min_ngram`.

## Guidance

Reach for [`HashMapper`](HashMapper.md) over
[`LexiconMapper`](LexiconMapper.md) when the vocabulary is unbounded or
unknown ahead of time (streaming text, user-generated content) and a fixed
memory footprint matters more than being able to map a column index back
to a term. Pick `n_features` comfortably larger than the number of
distinct terms you actually expect — collisions are silent, so a width
too small for the real vocabulary degrades quality without ever
signaling an error. Turn on `alternate_sign` whenever collisions are
likely (small `n_features` relative to vocabulary size) to keep them from
biasing column magnitudes upward.

## See Also

- [`HashMapper`](HashMapper.md) — the sole consumer.
- [`LexiconMapOptions`](LexiconMapOptions.md) — the vocabulary-backed
  counterpart, sharing this struct's tokenization fields exactly.
