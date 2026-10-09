# `pluss::ai::nlp::vectorize::Mapper`

A learned vectorizer. It reads a corpus once, fixes a vocabulary of terms and their document
frequencies, and then turns any document into a row over that vocabulary: counts, binary
presences, or TF-IDF weights, dense or sparse. The vocabulary is the model's column identity, so a
mapper must be learned before it maps anything, and the same mapper must produce both the training
and the inference matrices.

## Signature

```cpp
pluss::ai::nlp::vectorize::Mapper::Mapper(
    pluss::ai::classic::LexiconMapOptions map_options = {},
    pluss::ai::nlp::vectorize::DocMapOptions doc_options = {});
```

## Construction

The constructor takes two option structs. `map_options` is the classic vectorizer's configuration
([`LexiconMapOptions`](../../classic/text/LexiconMapOptions.md)): lowercasing, the vocabulary cap,
the minimum document frequency, binary counts, and the n-gram range. `doc_options` is the
[`DocMapOptions`](DocMapOptions.md) that decides which tokens become terms. The two are combined
at construction: the mapper's `keep_punct` follows `doc_options.skip_punct`, so the document option
is the single decision about punctuation.

A new mapper is untrained. Its vocabulary is empty, and the mapping methods refuse to run until
`learn` or `load_state` has been called.

## State

A learned mapper holds its vocabulary, one entry per term in learned order, the document frequency
of each term, the number of documents it learned from, and the options it was built with. An
untrained mapper holds none of these. The state round-trips through `dump_state` and `load_state`.

## Methods

Each method has its own page.

| Method | Returns | Covered on |
| --- | --- | --- |
| `learn(...)` | `Status` | [`learn`](Mapper/learn.md) |
| `count_dense(docs)` | `StatusOr<FeatureBatch>` | [`count_dense`](Mapper/count_dense.md) |
| `count_sparse(docs)` | `StatusOr<SparseTripletMatrix>` | [`count_sparse`](Mapper/count_sparse.md) |
| `tfidf_dense(docs, options)` | `StatusOr<FeatureBatch>` | [`tfidf_dense`](Mapper/tfidf_dense.md) |
| `tfidf_sparse(docs, options)` | `StatusOr<SparseTripletMatrix>` | [`tfidf_sparse`](Mapper/tfidf_sparse.md) |
| `vocabulary() const` | `const std::vector<std::string>&` | [`vocabulary`](Mapper/vocabulary.md) |
| `dump_state() const` | `LexiconMapperState` | [`dump_state`](Mapper/dump_state.md) |
| `load_state(state)` | `Status` | [`load_state`](Mapper/load_state.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Mapper/Introduction
Mapper/count_dense
Mapper/count_sparse
Mapper/dump_state
Mapper/learn
Mapper/load_state
Mapper/tfidf_dense
Mapper/tfidf_sparse
Mapper/vocabulary
```
<!-- apiref:toctree:end -->
