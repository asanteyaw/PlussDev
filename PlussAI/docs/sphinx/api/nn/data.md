# `pluss::loader`

`data` owns [`DataLoader`](data/DataLoader.md) and the train/val/test and
walk-forward splitting utilities purpose-built to feed it -- the
data-loading stack every training loop in this reference ultimately reads
batches from. It lives under `pluss::loader` (an alias of `pluss::tils`),
physically in **PlussTils**, not PlussAI -- included in this reference
because it is the data-loading half of the same training story
[`optim`](optim.md) is the other half of, not because it is part of the
`nn` engine itself.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Dataset`](data/Dataset.md) | the map-style dataset interface: `size()` plus `get(index)` |
| [`SeriesDataset`](data/SeriesDataset.md) | a `Dataset` backed directly by whole-`Series` fields -- the common case |
| [`DataBatch`](data/DataBatch.md) | one batch's aligned `Series` fields |
| [`DataLoader`](data/DataLoader.md) | batches a `Dataset` (or raw field `Series`) -- shuffling, threaded prefetch, a collate hook; iterable and index-addressable |
| [`Sampler`](data/Sampler.md) | the index-sampling strategy interface `DataLoaderOptions::sampler` takes, plus four concrete strategies |
| [`split_fields`](data/split_fields.md) / [`split_loaders`](data/split_loaders.md) | a one-shot random train/val/test split, as raw `Series` or wrapped in a `DataLoader` per part |
| [`temporal_split_indices`](data/temporal_split_indices.md) / [`temporal_split_fields`](data/temporal_split_fields.md) / [`temporal_split_loaders`](data/temporal_split_loaders.md) | forward-chaining (walk-forward) time-series cross-validation folds, at three levels: indices, `Series`, `DataLoader` |

## How To Call It

```cpp
namespace loader = pluss::loader;

const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4, 1});

loader::DataLoaderOptions options{};
options.batch_size = 2;
options.shuffle = true;

loader::DataLoader data_loader({x, y}, options);
for (auto batch : data_loader) {
  // batch.x() / batch.y() -- one Fusion forward + loss + optimizer.step() per batch
}
```

Every training example throughout this reference's [`optim`](optim.md)
family builds its own tiny in-example data by hand; `DataLoader` is what a
real training loop reaches for instead once data does not fit in one
`make_series` call.

## Related Pages

- [`optim`](optim.md) -- the optimizers a `DataLoader`-fed training loop
  steps each batch.
- [`grad`](grad.md) -- `DataLoader`'s own batch construction wraps itself
  in [`NoGradGuard`](grad/NoGradGuard.md), since selecting/collating a
  batch is bookkeeping that must never become part of a training graph.
- [`module`](module.md) -- the `Module`/`Schedule` a batch's fields are
  typically fed into.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

data/Introduction
data/Dataset
data/SeriesDataset
data/DataBatch
data/DataLoader
data/Sampler
data/SequentialSampler
data/RandomSampler
data/SubsetRandomSampler
data/WeightedRandomSampler
data/DataSplit
data/DataSplitOptions
data/split_fields
data/DataLoaderSplit
data/split_loaders
data/TemporalSplitOptions
data/temporal_split_indices
data/TemporalIndexSplit
data/temporal_split_fields
data/TemporalFieldSplit
data/temporal_split_loaders
data/TemporalLoaderSplit
```
<!-- apiref:toctree:end -->
