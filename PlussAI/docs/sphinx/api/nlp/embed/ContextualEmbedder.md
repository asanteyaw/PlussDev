# `pluss::ai::nlp::embed::ContextualEmbedder`

`ContextualEmbedder` is a causal Transformer text model trained without labels. It exposes sentence-dependent token rows, a mean-pooled document vector, and autoregressive prompt continuation from one learned state.

## Signature

```cpp
explicit pluss::ai::nlp::embed::ContextualEmbedder(
    pluss::ai::nlp::embed::ContextualEmbedOptions options = {});
```

Construction stores architecture and preprocessing options but does not initialize a usable learned model. [`learn()`](ContextualEmbedder/learn.md) trains replacement state; [`load()`](ContextualEmbedder/load.md) reconstructs a saved state after integrity checks.

## Methods

- [`embed`](ContextualEmbedder/embed.md) mean-pools contextual token rows.
- [`encode`](ContextualEmbedder/encode.md) returns one contextual row per surviving token.
- [`generate`](ContextualEmbedder/generate.md) continues a prompt greedily or by seeded sampling.
- [`learn`](ContextualEmbedder/learn.md) performs causal language-model pretraining.
- [`load`](ContextualEmbedder/load.md) verifies and restores an artifact.
- [`ready`](ContextualEmbedder/ready.md) reports whether learned state exists.
- [`save`](ContextualEmbedder/save.md) persists vocabulary, architecture, and parameters.
- [`width`](ContextualEmbedder/width.md) reports the contextual row width.

## Changelog

- **0.1** -- Documented contextual encoding, pooling, training, persistence, and generation.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

ContextualEmbedder/Introduction
ContextualEmbedder/embed
ContextualEmbedder/encode
ContextualEmbedder/generate
ContextualEmbedder/learn
ContextualEmbedder/load
ContextualEmbedder/ready
ContextualEmbedder/save
ContextualEmbedder/width
```
<!-- apiref:toctree:end -->
