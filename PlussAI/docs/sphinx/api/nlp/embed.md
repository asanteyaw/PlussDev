# `pluss::ai::nlp` Embeddings

The embedding family learns numerical text representations from unlabeled,
tokenized documents. It provides a static model for stable per-token vectors
and a contextual model whose token vectors depend on surrounding text.

## Model Choice

| Capability | [`Embedder`](embed/Embedder.md) | [`ContextualEmbedder`](embed/ContextualEmbedder.md) |
| --- | --- | --- |
| Token representation | One vector per vocabulary token | One vector per token occurrence |
| Training objective | Local context prediction | Causal next-token prediction |
| Document representation | Mean of known token vectors | Mean of contextual rows |
| Similarity / nearest terms | Yes | No vocabulary-neighbor API |
| Prompt continuation | No | Yes |
| Relative cost | Lower | Higher |

Both models train from scratch and own their learned state. Their `ready()`
queries distinguish a configured object from one that has completed training
or loading. `width()` is the vector dimension selected by options.

Static embedding queries return status-bearing values so unknown vocabulary,
untrained state, and invalid requests remain explicit. Contextual generation
uses [`GenerateOptions`](embed/GenerateOptions.md) to separate greedy decoding
from seeded sampling, temperature, and top-k restriction.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

embed/Introduction
embed/ContextualEmbedOptions
embed/ContextualEmbedder
embed/EmbedOptions
embed/Embedder
embed/GenerateOptions
```
<!-- apiref:toctree:end -->
