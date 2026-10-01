# `pluss::loader::SeriesDataset` Introduction

`SeriesDataset` is the concrete [`Dataset`](../Dataset.md) implementation
for the common case: data that already exists as one or more whole
`Series`, sharing a sample dimension. Unlike a custom `Dataset` subclass,
there is no `get`/`materialize_fields` to implement -- both are already
correct the moment the constructor validates its fields, since
`materialize_fields()` simply returns the exact fields the constructor
was given.

Constructing a [`DataLoader`](../DataLoader.md) directly from raw
`Series` fields (`DataLoader(fields, options)`) and constructing one from
a `SeriesDataset` built over those same fields
(`DataLoader(SeriesDataset(fields), options)`) produce identical batching
behavior -- `SeriesDataset` exists as an object you can pass around,
store, and reuse across multiple `DataLoader` constructions, not as a
functionally different path.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batch_dim.html">
    <span class="label">Next</span>
    <span class="title">batch_dim</span>
  </a>
</div>
<!-- apiref:nav:end -->
