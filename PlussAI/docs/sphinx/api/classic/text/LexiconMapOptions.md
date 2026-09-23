# `pluss::ai::classic::LexiconMapOptions`

Configures [`LexiconMapper`](LexiconMapper.md)'s vocabulary construction and
per-document tokenization — everything that shapes *what* the learned
vocabulary contains and *how* a document is broken into terms against it.
Set once at construction; `learn()` reads every field below and rejects
several of them outright before touching any document — see
[`LexiconMapper::learn`](LexiconMapper.md#methods).

## Fields

### `lowercase`

`bool lowercase = true`

Whether every document is ASCII-lowercased before tokenization, both at
[`learn`](LexiconMapper.md#methods) time and at every later `map_*` call.
`false` makes matching case-sensitive — `"Data"` and `"data"` become two
distinct vocabulary entries. See
[`LexiconMapper`'s Example 1](LexiconMapper.md#example-1) (default,
lowercased) and its case-sensitive counterpart.

### `max_features`

`std::int64_t max_features = 4096`

The largest vocabulary [`learn`](LexiconMapper.md#methods) will keep. After
every term passing `min_document_frequency` is sorted by document frequency
descending (ties broken alphabetically ascending), only the top
`max_features` survive — a term just past the cutoff is silently dropped,
never an error. Must be `> 0` — `learn`/`load_state` both reject `<= 0`. See
[`LexiconMapper`'s Example 4](LexiconMapper.md#example-4) for the exact
ranking and cutoff.

### `min_document_frequency`

`std::int64_t min_document_frequency = 1`

The minimum number of *documents* (not occurrences) a term must appear in
to enter the vocabulary — each document's own terms are deduplicated before
counting, so five occurrences in one document count once toward this floor.
The default `1` accepts every term that appears anywhere at all; raising it
filters out corpus-rare terms. If **no** term clears the floor,
[`learn`](LexiconMapper.md#methods) fails outright rather than returning an
empty vocabulary. Must be `> 0`. See
[`LexiconMapper`'s Example 3](LexiconMapper.md#example-3).

### `binary`

`bool binary = false`

`false` (the default): `map_dense`/`map_sparse` store each term's raw count
per document. `true`: store `1.0` on any occurrence, however many times a
term repeats. This also reaches
[`map_tfidf_dense`/`map_tfidf_sparse`](LexiconMapper.md#methods) indirectly
— both build on the same raw counts, so `binary = true` collapses every
document's term frequency to `0`/`1` before IDF weighting is even applied.
See [`LexiconMapper`'s Example 5](LexiconMapper.md#example-5).

### `min_ngram`

`std::int64_t min_ngram = 1`

The shorter end of the inclusive n-gram-length range extracted from each
document's token sequence (space-joined, e.g. `"data science"` for a
bigram) — see `max_ngram` below for the longer end and the full range's
behavior. Applies identically at `learn` and at every `map_*` call. Must
be `> 0`, and `<= max_ngram` — `learn`/`load_state` reject otherwise. If
`min_ngram` itself exceeds a document's own token count, that document
contributes **no** grams at all, silently. See
[`LexiconMapper`'s Example 7](LexiconMapper.md#example-7).

### `max_ngram`

`std::int64_t max_ngram = 1`

The longer end of the `min_ngram`/`max_ngram` range above. Silently capped
per document at that document's own token count — a short document simply
contributes fewer (or zero) grams at the longer lengths, never an error.
Must be `> 0`. See
[`LexiconMapper`'s Example 6](LexiconMapper.md#example-6).

## Guidance

Leave `min_document_frequency` at its default `1` unless the corpus is
large enough that singleton terms are mostly noise (typos, rare proper
nouns) rather than signal — raising it trades vocabulary size for
robustness. Reach for `binary = true` when a term's mere presence matters
more than how often it repeats (short documents, or a downstream linear
model where raw counts would let one repeated word dominate a row's norm).
`min_ngram`/`max_ngram` above `1` grow the vocabulary combinatorially with
document length; pair a wider range with a smaller `max_features` to keep
the result usable.

## See Also

- [`LexiconMapper`](LexiconMapper.md) — the sole consumer.
- [`TfIdfMapOptions`](TfIdfMapOptions.md) — the separate, per-call options
  struct for `map_tfidf_dense`/`map_tfidf_sparse`'s IDF weighting, layered
  on top of the raw counts these fields shape.
- [`HashMapOptions`](HashMapOptions.md) — the stateless counterpart's own
  tokenization options, sharing `lowercase`/`min_ngram`/`max_ngram`'s exact
  meaning but with no vocabulary to cap or filter.
