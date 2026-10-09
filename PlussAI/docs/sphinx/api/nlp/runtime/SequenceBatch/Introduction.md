# Working With Sequence Batches

A `SequenceBatch` is the point where text stops being text. Each row is one document's sequence, padded
to the longest row, and the masks and lengths tell every later layer which entries are real. Nothing
downstream re-derives those facts, so a batch built once is valid for the embedding, the encoder, and the
pooling step alike.

The padded layout is what makes a batch cheap to process: a whole mini-batch is one tensor. The cost is
that padding must be ignored, and the batch carries the masks that let every layer do so. Recurrent layers
can instead take the packed form that [`pack`](pack.md) produces, which skips the padding
entirely.

Build batches with the same [`Indexer`](../Indexer.md) that learned the vocabulary. A batch from a different
indexer refers to ids that mean something else.

## See Also

- [`empty`](empty.md), [`pack`](pack.md), and [`causal_mask`](causal_mask.md) -- the three methods.
- [`Indexer`](../Indexer.md) -- the source of every batch.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="causal_mask.html">
    <span class="label">Next</span>
    <span class="title">causal_mask</span>
  </a>
</div>
<!-- apiref:nav:end -->
