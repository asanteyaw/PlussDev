# `pluss::loader::DataLoader`

Batches a [`Dataset`](Dataset.md) (or raw field `Series`) via
`index_select`, with shuffling or a custom [`Sampler`](Sampler.md),
drop-last, reshuffle-per-epoch, threaded prefetch, and a collate hook.
Iterable (range-for, or `next()` one batch at a time) and
index-addressable (`batch_at`).

## Signature

```cpp
pluss::loader::DataLoader::DataLoader() = default;
explicit pluss::loader::DataLoader::DataLoader(const pluss::loader::Dataset& dataset, pluss::loader::DataLoaderOptions options = {});
explicit pluss::loader::DataLoader::DataLoader(const pluss::loader::SeriesDataset& dataset, pluss::loader::DataLoaderOptions options = {});
explicit pluss::loader::DataLoader::DataLoader(std::vector<pluss::Series> fields, pluss::loader::DataLoaderOptions options = {});
pluss::loader::DataLoader::DataLoader(std::initializer_list<pluss::Series> fields, pluss::loader::DataLoaderOptions options = {});

pluss::loader::DataLoader::DataLoader(const pluss::loader::DataLoader&) = delete;
pluss::loader::DataLoader& pluss::loader::DataLoader::operator=(const pluss::loader::DataLoader&) = delete;
pluss::loader::DataLoader::DataLoader(pluss::loader::DataLoader&&) noexcept;
pluss::loader::DataLoader& pluss::loader::DataLoader::operator=(pluss::loader::DataLoader&&) noexcept;
```

## Construction

```cpp
namespace loader = pluss::loader;

// raw fields directly -- the common path
loader::DataLoader a({x, y}, loader::DataLoaderOptions{.batch_size = 32, .shuffle = true});

// from a Dataset (SeriesDataset, or any custom subclass -- materialize_fields()
// is the fast path when overridden, get() is the fallback otherwise)
loader::SeriesDataset dataset({x, y});
loader::DataLoader b(dataset);

// default-constructed, empty -- typically followed by set_options() or assignment
loader::DataLoader c;
```

`DataLoader` is move-only (copy is deleted) -- its own worker threads, when
`num_workers > 0`, capture `this` internally, so copying would leave two
objects racing over the same background state; moving safely transfers
ownership instead (stopping the source's workers first).

## State

`empty()` / `field_count()` / `sample_count()` / `batch_size()` / `options()`
: Trivial `noexcept` accessors, not covered on their own pages, matching
  [`Module::is_training`](../module/Module/train.md)'s own precedent for
  one-line accessors: `empty()` is `sample_count() == 0`; `field_count()`
  is how many fields this loader batches; `sample_count()` is the total
  sample count (independent of batching); `batch_size()` is
  `options().batch_size`; `options()` returns the full
  [`DataLoaderOptions`](DataLoaderOptions.md) this loader was last
  constructed or `set_options`-ed with.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `next()` | `optional<DataBatch>` | [`next`](DataLoader/next.md) |
| `batch_at(index)` | `DataBatch` | [`batch_at`](DataLoader/batch_at.md) |
| `begin()` / `end()` | `iterator` | [`begin`](DataLoader/begin.md) / [`end`](DataLoader/end.md) |
| `reset(reshuffle = true)` | `DataLoader&` | [`reset`](DataLoader/reset.md) |
| `set_epoch(epoch, reshuffle = true)` | `DataLoader&` | [`set_epoch`](DataLoader/set_epoch.md) |
| `set_options(options)` | `DataLoader&` | [`set_options`](DataLoader/set_options.md) |
| `batch_count()` | `size_t` | [`batch_count`](DataLoader/batch_count.md) |
| `sample_count()` | `int64_t` | [`sample_count`](DataLoader/sample_count.md) |
| `batch_size()` | `int64_t` | [`batch_size`](DataLoader/batch_size.md) |
| `epoch()` | `uint64_t` | [`epoch`](DataLoader/epoch.md) |

## Changelog

- **0.1** — batching over `Dataset`/raw fields, with shuffle/sampler,
  drop-last, reshuffle-per-epoch, a collate hook, threaded prefetch
  (`num_workers`/`prefetch_factor`/`persistent_workers`/`pin_memory`/
  `timeout`/`in_order`/`worker_init_fn`), and exception propagation from
  worker threads.

## See Also

- [`Dataset`](Dataset.md) / [`SeriesDataset`](SeriesDataset.md) -- the
  sources this batches.
- [`Sampler`](Sampler.md) -- the pluggable index-order strategy.
- [`DataBatch`](DataBatch.md) -- what iteration produces.
- [`split_loaders`](split_loaders.md) /
  [`temporal_split_loaders`](temporal_split_loaders.md) -- construct a
  `DataLoader` per train/val/test part or fold directly.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

DataLoader/Introduction
DataLoader/batch_at
DataLoader/batch_count
DataLoader/batch_size
DataLoader/begin
DataLoader/end
DataLoader/epoch
DataLoader/next
DataLoader/reset
DataLoader/sample_count
DataLoader/set_epoch
DataLoader/set_options
DataLoaderOptions
```
<!-- apiref:toctree:end -->
