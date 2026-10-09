# `pluss::ai::nlp::embed::Embedder`

`Embedder` is a trainable static-vector model. It learns one dense row per vocabulary term with skip-gram negative sampling, then serves inexpensive token lookup, cosine comparison, nearest-term search, and mean-pooled document vectors.

## Signature

```cpp
explicit pluss::ai::nlp::embed::Embedder(
    pluss::ai::nlp::embed::EmbedOptions options = {});
```

Construction copies configuration but performs no training and allocates no learned vocabulary. Use [`learn()`](Embedder/learn.md) to replace the model state or [`load()`](Embedder/load.md) to restore it. A ready model owns its vocabulary and host vectors; returned vectors are copies, while `vocabulary()` is a borrowed read-only reference valid while the model state is unchanged.

## Methods

- [`contains`](Embedder/contains.md) tests exact post-policy vocabulary membership.
- [`embed`](Embedder/embed.md) mean-pools one document or maps a document batch.
- [`learn`](Embedder/learn.md) trains a replacement model from prepared documents.
- [`load`](Embedder/load.md) verifies and restores a saved artifact.
- [`nearest`](Embedder/nearest.md) ranks vocabulary neighbors by cosine similarity.
- [`ready`](Embedder/ready.md) reports whether learned state exists.
- [`save`](Embedder/save.md) writes a checksummed model artifact.
- [`similarity`](Embedder/similarity.md) computes cosine similarity for two host vectors.
- [`vector_of`](Embedder/vector_of.md) copies one learned term vector.
- [`vocabulary`](Embedder/vocabulary.md) exposes vocabulary rows in model order.
- [`width`](Embedder/width.md) reports the configured vector dimension.

## Changelog

- **0.1** -- Documented the complete static embedding lifecycle and query surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Embedder/Introduction
Embedder/contains
Embedder/embed
Embedder/learn
Embedder/load
Embedder/nearest
Embedder/ready
Embedder/save
Embedder/similarity
Embedder/vector_of
Embedder/vocabulary
Embedder/width
```
<!-- apiref:toctree:end -->
