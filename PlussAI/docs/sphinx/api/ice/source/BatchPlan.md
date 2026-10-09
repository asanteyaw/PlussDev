# `pluss::ai::ice::BatchPlan`

Batch size, shuffle, and seed for a [`Source`](Source.md)'s own batch
materialization -- distinct from
[`TrainOptions::batch_size`/`shuffle`/`seed`](../options/TrainOptions.md),
which govern the model-facing loop instead. The two line up by default:
[`Coach::learn(model, x, y)`](../coach/Coach/learn.md) builds its
internal [`TensorSource`](TensorSource.md) from a `BatchPlan` populated
straight out of `TrainOptions`' own fields of the same name. They
diverge the moment a `Source` is constructed by hand -- passed directly
to [`Coach::learn`/`infer`/`assess`](../coach/Coach.md)'s `Source&`
overloads, or produced by
[`data::DataSet::source(learn_plan, eval_plan)`](../data/DataSet/source.md)
-- where `BatchPlan` is the only thing controlling batching, and
`TrainOptions`' own `batch_size`/`shuffle`/`seed` are not consulted at
all.

## Fields

### `batch_size`

`std::int64_t batch_size = 0`
: Rows per batch. `0` (the default) or any value `>=` the row count
  means one batch holding every row -- the shape every
  [`infer`](../coach/Coach/infer.md)/[`assess`](../coach/Coach/assess.md)
  call defaults to, since chunking only trades device memory for extra
  batches with no effect on the result.

### `shuffle`

`bool shuffle = false`
: Whether rows are drawn in a fresh random permutation (`true`) or
  left in their original order (`false`, the default) before being cut
  into batches. [`TensorSource`](TensorSource.md)'s own
  [`prepare`](Source/prepare.md) is the only place this is read --
  reshuffling never happens mid-run, only once, the first time
  `prepare()` runs.

### `seed`

`std::uint64_t seed = 0`
: The seed `shuffle`'s permutation draws from, for a reproducible
  shuffle order across runs with the same `seed`. Ignored when
  `shuffle` is `false`.

## Guidance

Leave every field at its default for a one-shot inference/assessment
`Source` -- `batch_size = 0` processes everything in a single pass, and
shuffling never matters when there is no gradient step to decorrelate.
For a hand-constructed training `Source`, mirror whatever
`TrainOptions::batch_size`/`shuffle`/`seed` the rest of the run uses
unless there is a specific reason for the `Source`'s own batching to
diverge from it (for example, a validation/test `BatchPlan` that is
deliberately never shuffled, the way
[`TensorSource`'s own `prepare`](Source/prepare.md) treats its
validation/test batches regardless of what `learn_plan` requested).

## See Also

- [`Source`](Source.md) -- the abstraction every `BatchPlan` configures.
- [`TensorSource`](TensorSource.md) / [`DataSetSource`](DataSetSource.md)
  -- the two built-in `Source` implementations that take a `BatchPlan`
  directly.
- [`TrainOptions`](../options/TrainOptions.md) -- the model-facing
  fields of the same name/meaning, which `Coach`'s own convenience
  overloads copy into a `BatchPlan` for their internally-built `Source`.
