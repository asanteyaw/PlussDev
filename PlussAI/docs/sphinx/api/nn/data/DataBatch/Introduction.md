# `pluss::loader::DataBatch` Introduction

`DataBatch` owns exactly one thing: `fields`, a flat, ordered list of
`Series`. It has no invariant beyond "however many fields it has" --
`DataLoader` and `Dataset::get` both produce batches with every field the
same length along the batch axis, but `DataBatch` itself never checks
this; it is a plain carrier, not a validating type.

`x()`/`y()` are pure convenience over `at(0)`/`at(1)`, for the extremely
common two-field (input, target) case -- reach for `at(index)` (or
`operator[]`) directly for a batch with more than two fields, or a batch
whose field order does not map cleanly to "input, target".

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="at.html">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>
<!-- apiref:nav:end -->
