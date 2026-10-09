# Loader API Reference

[Back to PlussTils API Reference](../api_reference.md) · [Loaders Manual](../manual/loaders.md)

The Loader reference owns public operations in `pluss::loader`: reading a
dataset in batches, choosing the order samples are visited, splitting a
dataset for training, and resolving file paths and roots a loader reads from.

## Families

- [`DataBatch`](loader-data-batch.md): the batch type every loader and dataset works with.
- [`Dataset` and `SeriesDataset`](loader-dataset.md): the map-style interface a loader reads from.
- [`DataLoader`](loader-data-loader.md): construction, with its member pages for
  [accessors](loader-data-loader/accessors.md), [iteration](loader-data-loader/iteration.md),
  and [reset and epoch](loader-data-loader/reset-and-epoch.md).
- [`DataLoaderOptions`](loader-data-loader-options.md): every field, grouped by what it configures.
- [`Sampler`](loader-sampler.md): the four sampling strategies a loader can use instead of a plain shuffle.
- [Splitting](loader-split.md): `split_fields` and `split_loaders`, a ratio-based, shuffled partition.
- [Temporal splitting](loader-temporal-split.md): `temporal_split_indices`, `temporal_split_fields`,
  and `temporal_split_loaders`, rolling-origin folds for time-ordered data.
- [`PathContext` and `UrlContext`](loader-path-context.md): a root held independently of the process-wide one.
- [Paths and roots](loader-paths.md): the process-wide root and the functions that resolve paths under it.

For a guide with worked programs, see [Loaders: A Guide](../manual/loaders.md).

```{toctree}
:maxdepth: 1
:hidden:

loader-data-batch
loader-dataset
loader-data-loader
loader-data-loader/accessors
loader-data-loader/iteration
loader-data-loader/reset-and-epoch
loader-data-loader-options
loader-sampler
loader-split
loader-temporal-split
loader-path-context
loader-paths
```
