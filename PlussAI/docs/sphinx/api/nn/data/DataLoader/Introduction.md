# `pluss::loader::DataLoader` Introduction

`DataLoader` owns turning a source of samples -- raw `Series` fields, or
a [`Dataset`](../Dataset.md) -- into a sequence of batches. Three things
it decides every epoch: *which* samples go in which batch (plain
sequential order, `shuffle`, or a custom [`Sampler`](../Sampler.md)),
*how many* at a time (`batch_size`, `drop_last`), and *how* each batch is
actually computed (synchronously on the calling thread by default, or via
a background thread pool when `num_workers > 0`).

## Two Ways To Consume A `DataLoader`

- **Range-for** (`for (auto batch : data_loader)`) -- the common case.
  Calls [`begin`](begin.md)/[`end`](end.md) internally, which (when
  `reshuffle_each_epoch` is on, the default) reshuffles and resets the
  cursor for you at the start of each loop.
- **`next()`** -- pull one batch at a time by hand, returning
  `std::nullopt` once the epoch is exhausted. Useful when a loop needs to
  interleave batch consumption with other logic that does not fit a
  plain range-for, or when manually driving multiple loaders in lockstep.

Both ultimately call the same internal batch-construction path -- neither
is faster or more "correct" than the other; pick based on which control
flow shape the surrounding code needs.

## Threaded Prefetch

Setting `DataLoaderOptions::num_workers > 0` moves batch construction off
the calling thread entirely: a pool of background threads computes
batches ahead of consumption, bounded by `prefetch_factor` batches per
worker, so the calling thread's own work (a training step) overlaps with
the *next* batch already being prepared. This is a native thread pool, not
separate OS processes -- C++ has no interpreter-level lock forcing
process-level parallelism the way some other language runtimes need, so a
thread pool avoids the serialization/inter-process-communication cost a
process-based design would pay, while still delivering the same
pipeline-overlap benefit. `num_workers = 0` (the default) computes every
batch synchronously on the calling thread, exactly as if the threading
options did not exist.

## Related Pages

- [`Dataset`](../Dataset.md) / [`SeriesDataset`](../SeriesDataset.md) --
  what a `DataLoader` batches from.
- [`Sampler`](../Sampler.md) -- the pluggable alternative to plain
  `shuffle`.
- [`grad`](../../grad.md) -- `DataLoader`'s own batch construction runs
  under [`NoGradGuard`](../../grad/NoGradGuard.md).
- [`split_loaders`](../split_loaders.md) /
  [`temporal_split_loaders`](../temporal_split_loaders.md) -- build a
  `DataLoader` per train/val/test part or fold directly, without
  constructing each one by hand.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batch_at.html">
    <span class="label">Next</span>
    <span class="title">batch_at</span>
  </a>
</div>
<!-- apiref:nav:end -->
