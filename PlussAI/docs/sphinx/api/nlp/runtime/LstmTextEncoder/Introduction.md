# Recurrence Over A Sequence

The LSTM reads a sequence one element at a time and carries a state forward. Packing means the recurrence
never runs over padding, so a short sentence's final state is the state after its last real element, not
after the padding that follows it in the batch. The output is unpacked to the padded layout, and the padded
positions are zero.

A recurrent encoder costs time in proportion to the sequence length, which suits long inputs. Its input
width must equal the embedding width and its `batch_first` must equal the batch's.

## See Also

- [`forward`](forward.md) -- the encoding methods.
- [`model`](model.md) -- the underlying LSTM.

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
