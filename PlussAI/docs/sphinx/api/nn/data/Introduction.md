# `nn::data` Introduction

## What This Category Owns

`data` owns the data-loading stack a training loop reads batches from:

- [`Dataset`](Dataset.md) / [`SeriesDataset`](SeriesDataset.md) -- the
  map-style dataset contract, and the common concrete case backed
  directly by whole `Series` fields.
- [`DataBatch`](DataBatch.md) -- one batch's aligned `Series` fields.
- [`DataLoader`](DataLoader.md) -- batches a dataset (or raw fields), with
  shuffling, a custom [`Sampler`](Sampler.md), threaded prefetch, and a
  collate hook.
- [`Sampler`](Sampler.md) and its four concrete strategies
  ([`SequentialSampler`](SequentialSampler.md),
  [`RandomSampler`](RandomSampler.md),
  [`SubsetRandomSampler`](SubsetRandomSampler.md),
  [`WeightedRandomSampler`](WeightedRandomSampler.md)) -- pluggable
  per-epoch index ordering.
- [`split_fields`](split_fields.md) / [`split_loaders`](split_loaders.md)
  -- a one-shot random train/val/test split.
- [`temporal_split_indices`](temporal_split_indices.md) /
  [`temporal_split_fields`](temporal_split_fields.md) /
  [`temporal_split_loaders`](temporal_split_loaders.md) -- forward-chaining
  (walk-forward) time-series cross-validation, the split a shuffled
  train/val/test division would silently invalidate by training on future
  data.

## Where This Lives

Unlike every other category in this reference, `data` is not part of the
`nn` engine itself -- it lives under `pluss::loader` (an alias of
`pluss::tils`), physically in **PlussTils**. It is documented here because
it is the data-loading half of the same training story
[`optim`](../optim.md) is the other half of: a `DataLoader` produces
batches, a `Module`'s `forward` and a loss function turn a batch into a
loss, and an `Optimizer::step` closure turns that loss into an update.
Every symbol on this page's own examples spells the full `pluss::loader::*`
path, matching the convention `pluss::ai::nn::*` uses elsewhere in this
reference, even though the two do not share a namespace root.

## `Dataset` vs. Raw `Series` Fields

`DataLoader` accepts three different sources of data, and picking the
right one matters:

- **Raw `Series` fields directly** (`DataLoader(fields, options)`) -- the
  simplest path, and the one every example on this page's own reading
  order uses first: your data already exists as one or more whole
  `Series`, sharing a sample dimension, and `DataLoader` slices batches
  out of them via `index_select`.
- **[`SeriesDataset`](SeriesDataset.md)** -- functionally almost identical
  to passing raw fields directly (it *is* a thin `Dataset` wrapper around
  the same whole-`Series`-fields idea), but as an object you can pass
  around, store, and construct once and reuse.
- **A custom [`Dataset`](Dataset.md) subclass** -- override
  `materialize_fields()` for the fast, vectorized path (one
  `index_select` per batch, the same path every other source in this
  family uses); leave it at its default and `DataLoader` falls back to
  calling `get(index)` once per sample and assembling the batch from
  those -- slower, but means a subclass that only implements `size`/`get`
  (the minimal contract every `Dataset` has to satisfy regardless) is
  usable with `DataLoader` immediately, with no extra method required.
  Override `materialize_fields()` once a subclass's data can be produced
  as whole `Series` up front and per-sample `get()` calls become the
  bottleneck.

## Reading Order

1. [`DataLoader`](DataLoader.md) -- start here; its own `Introduction`
   and method pages cover construction, iteration, and every option.
2. [`Sampler`](Sampler.md) -- once `DataLoader`'s plain `shuffle` bool is
   familiar, this is the pluggable alternative for anything more specific
   (a fixed subset, weighted sampling, ...).
3. [`Dataset`](Dataset.md) / [`SeriesDataset`](SeriesDataset.md) --
   needed only when data does not already exist as plain `Series` fields.
4. [`split_fields`](split_fields.md) /
   [`temporal_split_indices`](temporal_split_indices.md) -- reach for
   these before training even starts, to produce the fields or folds a
   `DataLoader` is then constructed from.

## Related Pages

- [`optim`](../optim.md) -- the optimizers a `DataLoader`-fed training
  loop steps.
- [`grad`](../grad.md) -- `DataLoader`'s own batch construction runs under
  [`NoGradGuard`](../grad/NoGradGuard.md).
- [`losses`](../losses.md) -- the loss functions a training loop calls on
  a batch's `x()`/`y()` fields.
