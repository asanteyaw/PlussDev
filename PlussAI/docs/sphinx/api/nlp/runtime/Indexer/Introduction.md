# The Indexer's Vocabulary

An indexer's vocabulary is fixed when it learns, and every id it hands out is a position in that list.
Learning is therefore the only moment the vocabulary can grow. A document that arrives later is mapped
against the frozen list, and any element the list does not hold becomes the unknown id.

The special ids come first and in a fixed order: pad at `0`, unknown at `1`, then the boundary tokens if
they are enabled. Keeping those positions fixed means a model can rely on `0` meaning padding no matter
which corpus produced the vocabulary.

Padding is applied at batching time, not learning time. Each batch is as long as its longest sequence, so
a batch of short documents stays short. The masks and lengths the batch carries are what let later layers
ignore the padding.

## See Also

- [`learn`](learn.md) -- builds the vocabulary.
- [`batch`](batch.md) -- maps documents to ids.
- [`SequenceOptions`](../SequenceOptions.md) -- the choices that shape the vocabulary.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batch.html">
    <span class="label">Next</span>
    <span class="title">batch</span>
  </a>
</div>
<!-- apiref:nav:end -->
