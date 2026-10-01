# `pluss::loader::Dataset` Introduction

`Dataset` owns the map-style contract a data source implements to be
usable with [`DataLoader`](../DataLoader.md): "how many samples" (`size`)
and "give me sample `i`" (`get`), plus an opt-in vectorized-batching hook
(`materialize_fields`) and the axis samples run along
(`batch_dim`). Subclassing it directly is the path to reach for only when
data does not already exist as whole `Series` fields -- most data in
practice does, and [`SeriesDataset`](../SeriesDataset.md) already covers
that case without writing a subclass at all.

`DataLoader(const Dataset&, options)` tries `materialize_fields()` first:
when a subclass overrides it, that becomes every batch's source, via one
vectorized `index_select` per batch -- the fast path. When it is left at
its unimplemented (empty) default, `DataLoader` falls back to calling
`get(index)` once per sample (at construction, to build the equivalent
whole-`Series` fields) instead -- slower, since it pays one call per
sample rather than one call per batch, but it means the minimal
`size`/`get`-only subclass every `Dataset` is already required to
implement is immediately usable with `DataLoader`, with nothing else to
override. See [`materialize_fields`](materialize_fields.md) for exactly
when each path runs.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batch_dim.html">
    <span class="label">Next</span>
    <span class="title">batch_dim</span>
  </a>
</div>
<!-- apiref:nav:end -->
