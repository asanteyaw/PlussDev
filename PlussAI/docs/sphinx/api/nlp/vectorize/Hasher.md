# `pluss::ai::nlp::vectorize::Hasher`

A fixed-width vectorizer that needs no vocabulary. It maps each term to one of `n_features` columns by
a stable hash, so the feature width is set in advance, no training is needed, and an open or streamed
vocabulary costs nothing to accept. The price is that two terms may share a column, so the columns
cannot be named.

## Signature

```cpp
pluss::ai::nlp::vectorize::Hasher::Hasher(
    pluss::ai::classic::HashMapOptions map_options = {},
    pluss::ai::nlp::vectorize::DocMapOptions doc_options = {});
```

## Construction

The constructor takes the classic hashing configuration
([`HashMapOptions`](../../classic/text/HashMapOptions.md)): lowercasing, the feature width
`n_features`, binary counts, signed hashing, and the n-gram range. It takes the same
[`DocMapOptions`](DocMapOptions.md) as the mapper, and sets the hashing analyzer's punctuation
handling from `skip_punct`, so both options take effect in the same way.

A hasher is ready as soon as it is constructed. There is nothing to learn.

## State

A hasher has no learned state. Its behavior is fixed by its options, and a hasher built with the same
options maps the same text to the same columns in every process, because the hash is a fixed function
of the term's bytes. There is therefore nothing to save.

## Methods

Each method has its own page. Each comes in a document form and a text form.

| Method | Returns | Covered on |
| --- | --- | --- |
| `dense(...)` | `StatusOr<FeatureBatch>` | [`dense`](Hasher/dense.md) |
| `sparse(...)` | `StatusOr<SparseTripletMatrix>` | [`sparse`](Hasher/sparse.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Hasher/Introduction
Hasher/dense
Hasher/sparse
```
<!-- apiref:toctree:end -->
