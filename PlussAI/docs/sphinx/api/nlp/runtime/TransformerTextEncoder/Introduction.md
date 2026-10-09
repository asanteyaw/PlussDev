# Attention Over A Sequence

The transformer encoder lets every position read every other position in its sequence. Padding is excluded
from those reads by the key-padding mask the batch carries, so a short sentence is not diluted by the padding
beside it. Setting `causal` adds the causal mask, which turns the same encoder into a left-to-right model.

Attention costs grow with the square of the sequence length, so the encoder suits short and medium inputs.
Its width must equal the embedding width and its `batch_first` must equal the batch's, and both are checked
when a sequence reaches the encoder.

## See Also

- [`forward`](forward.md) -- the encoding methods.
- [`model`](model.md) -- the underlying neural encoder.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
