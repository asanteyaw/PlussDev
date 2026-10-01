# `pluss::loader::DataSplitOptions`

Train/val ratios (test is the remainder), shuffle, seed, and batch
dimension.

Passed to [`split_fields`](split_fields.md) and
[`split_loaders`](split_loaders.md); the default,
`DataSplitOptions{}`, is an `80/10/10` random train/val/test split, with
a fixed (`0`) seed.

## Fields

### `train_ratio`

`double train_ratio = 0.8`
: The fraction of samples assigned to `train_fields` --
  `floor(train_ratio * sample_count)` samples. Must be in `[0, 1]`.

### `val_ratio`

`double val_ratio = 0.1`
: The fraction of samples assigned to `val_fields` --
  `floor(val_ratio * sample_count)` samples, capped so
  `train_count + val_count` never exceeds `sample_count`. Must be in
  `[0, 1]`, and `train_ratio + val_ratio` must not exceed `1.0`. `0.0`
  produces an empty `val_fields` -- a plain train/test split.

### `shuffle`

`bool shuffle = true`
: When `true` (the default), which samples land in which part is a
  random permutation seeded from `seed`. `false` assigns samples in
  their original order: the first `train_count` to `train_fields`, the
  next `val_count` to `val_fields`, and the rest to `test_fields`.

### `seed`

`std::uint64_t seed = 0`
: The seed the `shuffle = true` permutation is derived from. The same
  `seed` on the same input always produces the identical split.
  Ignored when `shuffle` is `false`.

### `batch_dim`

`std::int64_t batch_dim = 0`
: Which axis of each field is the sample axis the split is taken along.
  `0` (the default) is correct for the common case of a leading sample
  axis. Overwritten automatically to the source's own `batch_dim()` when
  splitting a [`Dataset`](Dataset.md)/[`SeriesDataset`](SeriesDataset.md)
  directly, rather than raw fields.

## Guidance

Leave `shuffle = true` with a fixed `seed` for a reproducible random
split -- the common case for an i.i.d. dataset. Set `shuffle = false`
only when the sample order itself is meaningful and should be preserved
verbatim across the three parts (for genuinely time-ordered data, prefer
[`TemporalSplitOptions`](TemporalSplitOptions.md)'s forward-chaining
split instead, which never puts a later sample in `train_fields` and an
earlier one in `test_fields`). `val_ratio = 0.0` is the right choice when
no separate validation part is needed.

## See Also

- [`split_fields`](split_fields.md) / [`split_loaders`](split_loaders.md)
  -- the calls this configures.
- [`DataSplit`](DataSplit.md) -- the result `split_fields` returns.
- [`TemporalSplitOptions`](TemporalSplitOptions.md) -- the
  forward-chaining alternative for time-series data.
