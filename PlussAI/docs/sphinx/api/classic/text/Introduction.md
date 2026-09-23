# `classic::text` Introduction

## What This Family Owns

`text` owns two document vectorizers, sharing no code between them:
[`LexiconMapper`](LexiconMapper.md) learns a bounded vocabulary from a
corpus, then maps documents against it as counts, presence, or TF-IDF
weights; [`HashMapper`](HashMapper.md) needs no corpus at all — it hashes
every n-gram directly into a fixed-width output. Both can emit dense
([`FeatureBatch`](../data/FeatureBatch.md)) or sparse
([`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md)) output from
the same plain `std::string` documents, and both share the exact same
tokenization mechanics (`lowercase`, `min_ngram`/`max_ngram`) even though
their options structs are separate.

Like [`online`](../online/Introduction.md) and [`sparse`](../sparse/Introduction.md),
`text` is deliberately **not** part of the
[`estimators`](../estimators.md)/[`compose`](../compose.md)/
[`tune`](../tune.md)/[`io`](../io.md) ecosystem — neither class has a
`ModelSpec`, neither attaches to a [`Flow`](../compose/Flow.md). Their
output feeds directly into a downstream estimator instead: a
[`FeatureBatch`](../data/FeatureBatch.md) into anything under
[`estimators`](../estimators.md), or a
[`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md) straight into a
[`sparse`](../sparse.md) family learner without any conversion step.

## Vocabulary Vs. Hashing

The choice between the two classes is the single most consequential
decision in this family, and it is not about accuracy so much as about
what you need afterward:

- [`LexiconMapper`](LexiconMapper.md) produces an inspectable, bounded
  vocabulary (`vocabulary()`) — column `i` always means the same term, so
  a fitted linear model's weight vector can be read back term-by-term, and
  [`max_features`](LexiconMapOptions.md#max_features)/
  [`min_document_frequency`](LexiconMapOptions.md#min_document_frequency)
  give direct control over vocabulary size. It needs a `learn()` step
  before it can map anything, and the same vocabulary (via `dump_state`/
  `load_state`) must travel with the model to score new documents
  correctly later.
- [`HashMapper`](HashMapper.md) needs no corpus and no state at all — the
  same options always produce the same fixed-width output for any input,
  including a term it has never encountered. The cost is that a column no
  longer means one term: [`n_features`](HashMapOptions.md#n_features) too
  small relative to the real vocabulary size causes silent hash
  collisions, degrading quality without ever raising an error (see
  [`HashMapper`'s Example 3](HashMapper.md#example-3) for a real one,
  deliberately forced).

## Validate-Once Vs. Validate-Every-Call

`LexiconMapOptions` is validated exactly once, inside
[`learn`](LexiconMapper.md#methods) (or
[`load_state`](LexiconMapper.md#methods)) — every later `map_*` call trusts
that the options already in force are valid, since nothing can change them
in between. `HashMapOptions` has no such single validation point at all:
[`HashMapper`](HashMapper.md) has no `learn()`, so
[`n_features`](HashMapOptions.md#n_features) and the n-gram range are
checked fresh on **every** `map_dense`/`map_sparse` call instead — a
`HashMapper` built with invalid options constructs cleanly and simply
fails identically on every subsequent call, rather than failing once up
front. See [`HashMapper`'s Construction section](HashMapper.md#construction)
and [Example 7](HashMapper.md#example-7).

## TF-IDF Is Layered On Top Of `LexiconMapOptions::binary`

[`TfIdfMapOptions`](TfIdfMapOptions.md) governs only the IDF weighting and
row scaling; the term-frequency values it weights are the exact raw counts
`map_dense`/`map_sparse` would already produce — including whatever
[`LexiconMapOptions::binary`](LexiconMapOptions.md#binary) already did to
them. Setting `binary = true` on the *mapper's* construction-time options
therefore reaches all the way through TF-IDF too: every document's term
frequency collapses to `0`/`1` before
[`TfIdfMapOptions::sublinear_tf`](TfIdfMapOptions.md#sublinear_tf) or the
IDF multiply ever run. There is no separate "binary" toggle on
`TfIdfMapOptions` itself — the two structs are not independent.

## Reading Order

1. [`LexiconMapOptions`](LexiconMapOptions.md) /
   [`TfIdfMapOptions`](TfIdfMapOptions.md) — the two structs
   [`LexiconMapper`](LexiconMapper.md) reads, at construction and per call
   respectively.
2. [`LexiconMapper`](LexiconMapper.md) — the richer of the two classes
   (vocabulary construction, four map variants, persistence).
3. [`HashMapOptions`](HashMapOptions.md) — the options
   [`HashMapper`](HashMapper.md) reads fresh on every call.
4. [`HashMapper`](HashMapper.md) — the stateless, fixed-width counterpart.

## Related Pages

- [`data`](../data.md) — [`FeatureBatch`](../data/FeatureBatch.md), one of
  the two output forms both classes here can produce.
- [`sparse`](../sparse.md) — [`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md)/
  [`SparseCsrMatrix`](../sparse/SparseCsrMatrix.md), this family's other
  output form, and the learners that consume it directly.
- [`estimators`](../estimators.md) — where a `FeatureBatch` produced here
  ordinarily goes next.
