# `pluss::loader::DataLoaderOptions`

Every knob a [`DataLoader`](DataLoader.md) reads: batch size, shuffle or
a custom [`Sampler`](Sampler.md), drop-last, the shuffle seed, which axis
is the batch dimension, reshuffle-per-epoch, an optional target
platform/dtype to move each batch to, an optional collate hook, and the
threaded-prefetch controls (`num_workers`, `prefetch_factor`,
`persistent_workers`, `pin_memory`, `timeout`, `in_order`,
`worker_init_fn`).

Passed to [`DataLoader`](DataLoader.md)'s constructor, or later via
[`set_options`](DataLoader/set_options.md); the default,
`DataLoaderOptions{}`, is a synchronous, unshuffled, `batch_size = 32`
loader with no platform/dtype conversion and no collation.

## Fields

### `batch_size`

`std::int64_t batch_size = 32`
: How many samples each batch targets. The last batch in an epoch holds
  fewer samples when `sample_count()` does not divide evenly by
  `batch_size`, unless `drop_last` is set. See
  [`DataLoader::batch_size`](DataLoader/batch_size.md).

### `shuffle`

`bool shuffle = false`
: When `true`, each epoch's sample order is a fresh permutation seeded
  from `seed + epoch`; when `false` (the default), samples are batched
  in ascending index order. Ignored when `sampler` is set -- the two are
  mutually exclusive.

### `drop_last`

`bool drop_last = false`
: When `true`, a final batch smaller than `batch_size` is discarded
  instead of delivered. `false` (the default) delivers it.

### `seed`

`std::uint64_t seed = 0`
: The base seed a `shuffle = true` permutation is derived from; the
  actual per-epoch seed is `seed + epoch()`, so the same `seed` and
  `epoch` always reproduce the same order. Ignored when `shuffle` is
  `false` or `sampler` is set.

### `batch_dim`

`std::int64_t batch_dim = 0`
: Which axis of each field is the sample axis that batching selects
  along -- passed straight to `index_select`. `0` (the default) is
  correct for the common case of a leading batch axis.

### `reshuffle_each_epoch`

`bool reshuffle_each_epoch = true`
: When `true` (the default), starting a new pass over the loader (a
  fresh range-for, or an explicit [`reset`](DataLoader/reset.md))
  advances `epoch()` by one and draws a new permutation when
  `shuffle` is on. `false` keeps the same order (and the same `epoch`)
  across passes until `epoch` is advanced explicitly via
  [`set_epoch`](DataLoader/set_epoch.md).

### `target_platform`

`std::optional<Platform> target_platform = std::nullopt`
: When set, every field of every produced batch is moved to this
  platform (CPU/CUDA/MPS) before delivery. Unset (the default) leaves
  each field on whatever platform it already lives on.

### `target_dtype`

`std::optional<ScalarType> target_dtype = std::nullopt`
: When set, every field of every produced batch is cast to this dtype
  before delivery. Unset (the default) leaves each field at its own
  dtype.

### `collate_fn`

`std::function<DataBatch(DataBatch)> collate_fn = nullptr`
: An optional hook run on each assembled batch before delivery --
  receives the batch `index_select` (and `target_platform`/
  `target_dtype`, when set) already produced, returns the batch actually
  delivered to the caller. Unset (the default) delivers the batch
  unchanged. Use it for per-batch transforms that only make sense once
  samples are already grouped (padding to a shared length, batch-level
  augmentation), rather than per-sample.

### `sampler`

`std::shared_ptr<Sampler> sampler = nullptr`
: A custom index-sampling strategy, overriding `shuffle` when set (the
  two are mutually exclusive). Unset (the default) keeps the plain
  `shuffle` bool's own behavior. See
  [`SequentialSampler`](SequentialSampler.md),
  [`RandomSampler`](RandomSampler.md),
  [`SubsetRandomSampler`](SubsetRandomSampler.md), and
  [`WeightedRandomSampler`](WeightedRandomSampler.md) for the built-in
  strategies.

### `num_workers`

`std::size_t num_workers = 0`
: How many background threads compute batches ahead of consumption. `0`
  (the default) computes every batch synchronously on the calling
  thread. `> 0` spawns that many worker threads -- real OS threads
  sharing this process's memory, not separate processes, so there is no
  serialization/IPC cost to pay for handing a batch back to the
  consumer.

### `prefetch_factor`

`std::optional<std::int64_t> prefetch_factor = std::nullopt`
: How many batches each worker may compute ahead of the consumer before
  blocking. Only meaningful when `num_workers > 0`; setting it with
  `num_workers == 0` throws. Unset (the default) resolves to `2` once
  `num_workers > 0`.

### `persistent_workers`

`bool persistent_workers = false`
: When `true`, worker threads started for one epoch are kept alive and
  reused for the next epoch instead of being joined and respawned.
  Requires `num_workers > 0`.

### `pin_memory`

`bool pin_memory = false`
: When `true`, calls `.pin_memory()` on every field of every produced
  batch. Currently a documented no-op in the underlying tensor engine --
  wiring it through here means real pinning takes effect automatically
  the moment the engine implements it, with no `DataLoader`-side change
  needed.

### `timeout`

`double timeout = 0.0`
: Seconds to wait for a prefetched batch before throwing. `0.0` (the
  default) waits indefinitely. Only meaningful when `num_workers > 0`.

### `in_order`

`bool in_order = true`
: When `true` (the default), batches are delivered in strict ascending
  index order even when `num_workers > 1` lets workers finish out of
  order. `false` delivers a batch as soon as any worker finishes it,
  which can improve throughput at the cost of a non-deterministic
  delivery order.

### `worker_init_fn`

`std::function<void(std::size_t)> worker_init_fn = nullptr`
: Called once per worker thread, from that thread, right after it starts
  and before it computes its first batch -- given the worker's own id in
  `[0, num_workers)`. A plain thread-entry hook; use it for thread-local
  setup a `collate_fn` might need. Unset (the default) runs no hook.

## Guidance

Leave every field at its default for a synchronous, unshuffled loader --
the common case. Set `shuffle = true` with a fixed `seed` for
reproducible training order, or supply `sampler` instead when the
built-in uniform-permutation shuffle is not the sampling strategy needed
(weighted resampling, a fixed subset, ...). `batch_size`/`drop_last`
govern how samples split into batches; `target_platform`/`target_dtype`
save a caller from converting each batch by hand when every consumer
wants the same platform/dtype. The `num_workers` group
(`prefetch_factor`/`persistent_workers`/`pin_memory`/`timeout`/
`in_order`/`worker_init_fn`) only matters once `num_workers > 0` --
setting any of them with `num_workers == 0` either throws
(`prefetch_factor`) or is simply inert.

## See Also

- [`DataLoader`](DataLoader.md) -- the class this configures.
- [`Sampler`](Sampler.md) -- the pluggable strategy `sampler` takes.
- [`DataLoader::set_options`](DataLoader/set_options.md) -- changes these
  options after construction.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

```
<!-- apiref:toctree:end -->
