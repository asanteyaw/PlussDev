# `pluss::ai::ice::models::AutoregressiveTrainOptions`

Batch size, learning rate, optimizer, shuffle/seed, and the shared
validation/regularization/checkpoint/execution policies for
[`AutoregressiveForecaster::learn`](AutoregressiveForecaster/learn.md)
-- the same field set and nested policy types as
[`pluss::ai::ice::TrainOptions`](../options/TrainOptions.md), under a
model-specific name.

## Fields

### `batch_size`

`std::int64_t batch_size = 64`
: How many windows each mini-batch targets when training from a
  single, unbatched [`AutoregressiveBatch`](AutoregressiveBatch.md)/
  [`data::Batch`](../data/Batch.md)/
  [`data::TimeSeriesBatch`](../data/TimeSeriesBatch.md).

### `learning_rate`

`double learning_rate = 1e-3`
: The optimizer's step size.

### `optimizer`

`OptimizerKind optimizer = OptimizerKind::Adam`
: Which `nn::optim` algorithm backs training -- same enum as
  [`TrainOptions::optimizer`](../options/TrainOptions.md).

### `shuffle`

`bool shuffle = true`
: Whether each epoch's mini-batches are drawn in a fresh random
  permutation.

### `seed`

`std::uint64_t seed = 0`
: Seeds `shuffle`'s permutation and the internal train/validation
  split (when `validation.split_ratio > 0`) -- independent of
  [`AutoregressiveForecasterOptions::seed`](AutoregressiveForecasterOptions.md),
  which seeds parameter initialization instead.

### `progress`

[`ProgressPolicy`](../options/ProgressPolicy.md) `progress = {}`
: Live completed-epoch reporting through the shared built-in logger. The
  default metric names show mean Gaussian-NLL training loss and validation loss
  when validation ran. Silent by default.

### `validation`

[`ValidationPolicy`](../options/ValidationPolicy.md) `validation = {}`
: Validation split ratio, evaluation cadence, and the monitored
  metric name/direction.

### `objective_regularization`

[`ObjectiveRegularization`](../options/ObjectiveRegularization.md) `objective_regularization = {}`
: Weight decay, L1/L2 penalties, and label smoothing applied to the
  training objective.

### `loop_regularization`

[`LoopRegularization`](../options/LoopRegularization.md) `loop_regularization = {}`
: Early stopping, patience, minimum delta, and best-weight
  restoration.

### `checkpoint`

[`CheckpointPolicy`](../options/CheckpointPolicy.md) `checkpoint = {}`
: Whether/when to write checkpoints during training, where, and how
  to resume.

### `execution`

[`ExecutionPolicy`](../options/ExecutionPolicy.md) `execution = {...}`
: Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
  failure, determinism, precision, and platform. Defaults to
  gradient clipping by norm at threshold `1.0`, `max_epochs = 60`,
  `fail_on_non_finite = true`, and `TrainPlatform::CPU` -- not the
  all-defaults `ExecutionPolicy{}` every other `*TrainOptions` in this
  manifest starts from.

## Guidance

Leave every field at its default for a reasonable first run; the one
difference from a bare `ExecutionPolicy{}` is that `execution` here
already arrives with gradient clipping and non-finite failure
pre-wired, matching
[`make_optimizer`](../optim/make_optimizer.md)'s own default. Set
`validation.split_ratio > 0` together with `loop_regularization` for
early stopping, exactly as [`TrainOptions`](../options/TrainOptions.md)
describes -- every nested policy struct here is the identical shared
type.

## See Also

- [`AutoregressiveForecaster::learn`](AutoregressiveForecaster/learn.md)
  -- the consumer.
- [`TrainOptions`](../options/TrainOptions.md) -- the generic
  equivalent this mirrors field-for-field.
- [`TFTTrainOptions`](TFTTrainOptions.md) -- the identical shape for
  `TemporalFusionTransformer`.
