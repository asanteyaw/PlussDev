# `pluss::ai::classic::TfIdfMapOptions`

Configures the IDF weighting and row scaling
[`LexiconMapper::map_tfidf_dense`/`map_tfidf_sparse`](LexiconMapper.md#methods)
apply on top of the raw term counts
[`LexiconMapOptions`](LexiconMapOptions.md) already shaped. Unlike
`LexiconMapOptions`, this struct is **not** fixed at construction — a fresh
value is passed to each `map_tfidf_*` call (default `{}` if omitted), so
the same learned vocabulary can be scored under different weighting
schemes without re-learning.

## Fields

### `smooth_idf`

`bool smooth_idf = true`

Which IDF formula is used, for every vocabulary term `t` with document
frequency `df(t)` (fixed at [`learn`](LexiconMapper.md#methods) time) and
corpus size `n` (the document count `learn` was called with):

- `true` (the default): `idf(t) = log((1 + n) / (1 + df(t))) + 1` — the
  `+1` in both numerator and denominator prevents division by zero and
  keeps every term's weight finite and positive, even a term present in
  every document (`df(t) == n`).
- `false`: `idf(t) = log((n + 1e-12) / (df(t) + 1e-12)) + 1` — the raw
  textbook formula, nudged by a tiny epsilon instead of `+1` smoothing so
  it never divides by exactly zero.

The two formulas agree closely for a mid-frequency term but diverge for
one near the corpus size or near `1`; see
[`LexiconMapper`'s Example 9](LexiconMapper.md#example-9) for both computed
and compared by hand against the real output.

### `sublinear_tf`

`bool sublinear_tf = false`

`false` (the default): the raw term frequency (or `0`/`1` if
[`LexiconMapOptions::binary`](LexiconMapOptions.md#binary) is set) is used
directly. `true`: every *positive* term frequency `tf` is replaced by
`1 + log(tf)` before IDF weighting — dampens the effect of a term that
repeats many times in one document, so a word appearing `20` times doesn't
dominate `10x` more than one appearing `2` times. A term absent from a
document (`tf == 0`) is left at `0` regardless of this flag — there is
nothing to dampen. See
[`LexiconMapper`'s Example 10](LexiconMapper.md#example-10).

### `l2_normalize`

`bool l2_normalize = true`

`true` (the default): each document's row is divided by its own Euclidean
(L2) norm after IDF weighting, so every non-all-zero row has unit length —
the standard TF-IDF convention, making cosine similarity between two rows
equal to their plain dot product. `false`: rows are left at their raw
IDF-weighted magnitude. A document that matches **no** vocabulary term at
all is left as an all-zero row either way (dividing by an L2 norm of `0`
is skipped, not a division-by-zero error). See
[`LexiconMapper`'s Example 11](LexiconMapper.md#example-11).

## Guidance

Leave all three at their defaults for the ordinary case — smoothed IDF,
linear term frequency, L2-normalized rows is the standard TF-IDF
convention most downstream consumers (cosine similarity, a linear
classifier) expect. Reach for `sublinear_tf = true` on longer documents
where raw repetition count is a poor proxy for relevance (a word repeated
because a document is long, not because it is unusually important to that
document). Set `l2_normalize = false` only when a downstream consumer
needs the un-normalized magnitude itself — for instance, comparing
weighted term-count totals directly rather than a similarity measure.

## See Also

- [`LexiconMapper`](LexiconMapper.md) — the sole consumer, via
  `map_tfidf_dense`/`map_tfidf_sparse`.
- [`LexiconMapOptions`](LexiconMapOptions.md) — the fixed-at-construction
  struct governing the raw term counts and vocabulary this weighting is
  layered on top of, including `binary`, which changes what "term
  frequency" even means before this struct's fields ever run.
