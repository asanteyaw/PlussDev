# `pluss::ai::nlp::embed::ContextualEmbedder` Introduction

`ContextualEmbedder` combines an indexed vocabulary, token and learned position embeddings, a causally masked Transformer encoder, and an output projection head. `learn()` trains all of those parameters together by predicting each next token from the prefix to its left.

## Why Context Changes A Row

`encode()` performs a model forward pass over the complete input sequence. Attention lets each position incorporate the preceding context allowed by the causal mask, so two occurrences of the same vocabulary id need not have equal vectors. The returned rows exclude internal beginning/end markers and padding; their order follows the surviving content tokens.

`embed()` averages those rows. It is convenient when a downstream operation requires one vector per document, but averaging deliberately removes token positions. Keep `encode()` output for token classification, alignment, or occurrence-level analysis.

## Generation Uses The Same Objective

`generate()` reopens a framed prompt by removing its terminal marker, repeatedly encodes the growing prefix, projects the final row to vocabulary logits, and appends one selected token. It stops at an end marker, the requested budget, or learned position capacity. The return value contains only newly generated tokens.

## Lifecycle, Ownership, And Cost

A constructed model is not ready. Successful training or loading installs all required components together. Encoding is deterministic for fixed state and input; sampled generation is reproducible for a fixed decoding seed.

Training currently forms one padded full-corpus batch. That maximizes vectorized work but means memory depends on document count and the longest sequence. Encoding cost grows with sequence length and Transformer depth; generation repeats an encode for each produced token and is therefore substantially more expensive than static lookup.

## Vocabulary Boundaries

The internal indexer maps unseen terms to its unknown token rather than enlarging the vocabulary during inference. Preparation policy is restored with the indexer artifact. Empty prompts are valid for generation because the beginning marker supplies an initial position; empty content is not valid for `encode()` or `embed()` because no public token row exists to return or pool.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="embed.html">
    <span class="label">Next</span>
    <span class="title">embed</span>
  </a>
</div>
<!-- apiref:nav:end -->
