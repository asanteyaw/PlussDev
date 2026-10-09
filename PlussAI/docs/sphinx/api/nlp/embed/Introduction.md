# Embedding Concepts

The embedding family turns tokenized `Doc` values into dense numerical representations without requiring labels. It contains two deliberately different models. [`Embedder`](Embedder.md) learns one reusable vector for each vocabulary term. [`ContextualEmbedder`](ContextualEmbedder.md) runs every token occurrence through a causal Transformer, so the same spelling can receive a different vector in a different sentence.

That distinction determines which model belongs in a design. A static vector is inexpensive to store, retrieve, compare, and average. It is the right representation for vocabulary exploration, nearest-term lookup, compact document features, and workloads where a term should have stable identity. A contextual row carries sentence-specific information and is therefore better for ambiguity, word order, token-level downstream models, and prompt continuation, but encoding requires a full model forward pass.

## Input Contract

Both trainers consume `std::vector<Doc>`. Tokenization is intentionally outside the model: construct the documents with a [`Workflow`](../workflow/Workflow.md), then choose embedding options that agree with the annotations that workflow produced.

- `use_lemmas=true` selects `Token::lemma`; run a lemmatizer when inflected forms should share a vocabulary entry.
- `lowercase=true` normalizes the selected text before vocabulary lookup.
- `skip_punct=true` removes tokens marked `is_punct`.

These policies apply during training and inference. A model trained case-sensitively does not silently lowercase later queries, and a saved model restores the same preprocessing policy.

## Three Output Levels

The family exposes three different granularities rather than hiding them behind one ambiguous call:

| Need | Call | Result |
| --- | --- | --- |
| One static term | `Embedder::vector_of()` | one `std::vector<double>` of length `width()` |
| Every contextual occurrence | `ContextualEmbedder::encode()` | one width-sized row per surviving input token |
| One whole document | either model's `embed()` | the arithmetic mean of that model's token vectors |

Mean pooling makes document vectors comparable and easy to feed into classical models, but it discards token order and count. Use `encode()` when downstream work needs the individual contextual rows.

## Lifecycle And Reproducibility

Construction stores options but does not create a usable model. `ready()` remains false until `learn()` succeeds or `load()` restores a complete artifact. Training always starts from scratch; calling `learn()` again replaces the prior vocabulary and weights.

The seed fixes initialization, data shuffling, and negative sampling where applicable. Contextual generation has a separate seed because decoding randomness is an inference choice, not a training choice. Greedy decoding is deterministic and ignores sampling-only fields.

Persist trained models for reuse. `save()` writes architecture, vocabulary, preprocessing state, and learned parameters with a checksum; `load()` verifies the artifact before exposing a ready model.

## Choosing A Model

Use `Embedder` when direct vocabulary lookup or nearest-neighbor search is part of the job, or when inference cost must stay close to table lookup. Use `ContextualEmbedder` when occurrence-level meaning matters or when the model must continue a prompt. Neither model is a tokenizer, and neither invents missing annotations; the preparation workflow remains part of the model contract.

