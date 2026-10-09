# `pluss::ai::nlp::embed::Embedder` Introduction

An `Embedder` has two states. A newly constructed object is configured but untrained: `ready()` is false, `vocabulary()` is empty, and model-dependent queries return a failed `StatusOr`. Successful `learn()` or `load()` installs a complete state atomically and makes the object ready.

## What Training Learns

The trainer converts each prepared `Doc` to terms under [`EmbedOptions`](../EmbedOptions.md), counts terms, filters by `min_count`, and creates center/context pairs within `window`. For every positive pair it draws `negative_samples` noise terms and optimizes two embedding tables with binary cross-entropy. The public vectors are copied from the trained input table.

The result is static: `vector_of("bank")` returns the same coordinates regardless of which sentence supplied the token. That property makes lookup and nearest-term search cheap. If occurrence-specific meaning is required, use [`ContextualEmbedder`](../ContextualEmbedder.md).

## Ownership And Cost

The model owns vocabulary strings and vectors. `vector_of()` and `embed()` return independent host vectors. `vocabulary()` avoids a copy and therefore borrows the model's ordered vocabulary. A model copy shares the underlying immutable learned state until either object learns or loads replacement state.

`contains()` is a hash lookup. `vector_of()` adds a width-sized copy. Document embedding is linear in surviving token count times width. `nearest()` scans every vocabulary row and partially sorts the requested prefix, so it is suitable for direct exploration but is not an approximate large-vocabulary index.

## Preprocessing Consistency

The model does not tokenize raw strings. Prepare both training and inference text through a `Workflow`, and preserve the lemma, case, and punctuation assumptions selected in `EmbedOptions`. Saved artifacts carry these policies, so a restored model interprets documents exactly like the model that wrote them.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
<!-- apiref:nav:end -->

