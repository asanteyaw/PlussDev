# `pluss::loader::TemporalSplitOptions`

Fold count, explicit test size, max train size cap, the train/test gap,
and batch dimension for forward-chaining (walk-forward) time-series
cross-validation.

Passed to [`temporal_split_indices`](temporal_split_indices.md),
[`temporal_split_fields`](temporal_split_fields.md), and
[`temporal_split_loaders`](temporal_split_loaders.md); the default,
`TemporalSplitOptions{}`, produces `5` folds with an inferred test size
and no gap.

## Fields

### `folds`

`std::int64_t folds = 5`
: How many train/test folds to produce. Must be `>= 2`, and
  `folds + 1` must not exceed the sample count being split (one fold
  needs at least a training window plus a test window, and every
  earlier fold's test window also has to fit before it).

### `test_size`

`std::optional<std::int64_t> test_size = std::nullopt`
: How many samples each fold's `test_indices` holds. Unset (the
  default) infers `sample_count / (folds + 1)`. Must be `> 0` when
  provided.

### `max_train_size`

`std::optional<std::int64_t> max_train_size = std::nullopt`
: Caps how many samples a fold's `train_indices` can hold, keeping only
  the most recent `max_train_size` samples before that fold's test
  window. Unset (the default) lets `train_indices` grow to include
  every sample from `0` up to the gap boundary -- an expanding window.
  Set this for a fixed-size rolling window instead. Must be `> 0` when
  provided.

### `gap`

`std::int64_t gap = 0`
: How many samples to leave out between the end of `train_indices` and
  the start of `test_indices` for every fold. `0` (the default) trains
  right up to the test window with no buffer. Must be `>= 0`.

### `batch_dim`

`std::int64_t batch_dim = 0`
: Which axis of each field is the sample axis the split is taken along.
  `0` (the default) is correct for the common case of a leading sample
  axis. Overwritten automatically to the source's own `batch_dim()` when
  splitting a [`Dataset`](Dataset.md)/[`SeriesDataset`](SeriesDataset.md)
  directly, rather than raw fields.

## Guidance

Leave `test_size`/`max_train_size` unset for the common case: an
inferred, evenly-sized test window and an expanding training window.
Set `test_size` explicitly when a specific test-window length matters
more than an even split across `folds`. Set `max_train_size` when the
model should only ever train on a fixed-size recent window rather than
the whole growing history -- common for non-stationary series where
very old data actively hurts. Set `gap` when predictions must be made
some fixed number of steps ahead of the training cutoff, so the
evaluation reflects that lag rather than training right up to the test
window's first sample. Every fold here trains only on samples strictly
before its own test window (plus `gap`) -- this is what makes the split
safe for time series, unlike [`DataSplitOptions`](DataSplitOptions.md)'s
plain random split, which can leak future samples into training.

## See Also

- [`temporal_split_indices`](temporal_split_indices.md) /
  [`temporal_split_fields`](temporal_split_fields.md) /
  [`temporal_split_loaders`](temporal_split_loaders.md) -- the calls
  this configures.
- [`DataSplitOptions`](DataSplitOptions.md) -- the plain random-split
  alternative for i.i.d. data.
