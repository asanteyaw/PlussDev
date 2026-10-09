# `pluss::ai::ice::models::TFTTrainOptions`

Batch size, learning rate, optimizer, shuffle/seed, and the shared
validation/regularization/checkpoint/execution policies for
[`TemporalFusionTransformer::learn`](TemporalFusionTransformer/learn.md)
-- the same shape as
[`AutoregressiveTrainOptions`](AutoregressiveTrainOptions.md), under
its own model-specific name.

## Fields

### `batch_size`

`std::int64_t batch_size = 64`
: How many windows each mini-batch targets when training from a
  single, unbatched [`TFTBatch`](TFTBatch.md)/
  [`data::Batch`](../data/Batch.md)/
  [`data::TimeSeriesBatch`](../data/TimeSeriesBatch.md).

### `learning_rate`

`double learning_rate = 1e-3`
: The optimizer's step size.

### `optimizer`

`OptimizerKind optimizer = OptimizerKind::Adam`
: Which `nn::optim` algorithm backs training.

### `shuffle`

`bool shuffle = true`
: Whether each epoch's mini-batches are drawn in a fresh random
  permutation.

### `seed`

`std::uint64_t seed = 0`
: Seeds `shuffle`'s permutation and the internal train/validation
  split (when `validation.split_ratio > 0`) -- independent of
  [`TFTOptions::seed`](TFTOptions.md), which seeds parameter
  initialization instead.

### `progress`

[`ProgressPolicy`](../options/ProgressPolicy.md) `progress = {}`
: Live completed-epoch reporting. Enable `verbose`, choose ordinary lines or a
  terminal progress bar, select `loss`/`val_loss`, and control cadence exactly
  as on generic `TrainOptions`. Silent by default.

### `validation`

[`ValidationPolicy`](../options/ValidationPolicy.md) `validation = {}`
: Validation split ratio, evaluation cadence, and the monitored
  metric name/direction.

### `objective_regularization`

[`ObjectiveRegularization`](../options/ObjectiveRegularization.md) `objective_regularization = {}`
: Weight decay, L1/L2 penalties, and label smoothing applied to the
  pinball-loss training objective.

### `loop_regularization`

[`LoopRegularization`](../options/LoopRegularization.md) `loop_regularization = {}`
: Early stopping, patience, minimum delta, and best-weight
  restoration. Quantile *spread* calibration (as opposed to median
  accuracy) tends to need a more generous `max_epochs`/`patience`
  budget than a point forecast would -- see
  [`TemporalFusionTransformer::learn`'s own examples](TemporalFusionTransformer/learn.md).

### `checkpoint`

[`CheckpointPolicy`](../options/CheckpointPolicy.md) `checkpoint = {}`
: Whether/when to write checkpoints during training, where, and how
  to resume.

### `execution`

[`ExecutionPolicy`](../options/ExecutionPolicy.md) `execution = {...}`
: Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
  failure, determinism, precision, and platform. Defaults to
  gradient clipping by norm at threshold `1.0`, `max_epochs = 60`,
  `fail_on_non_finite = true`, and `TrainPlatform::CPU`.

## Guidance

Leave every field at its default for a reasonable first run.
Quantile-spread calibration under pinball loss converges more slowly
than median accuracy does -- if a forecast interval looks implausibly
narrow or wide after training, raise `execution.max_epochs` and
`loop_regularization.patience` together before suspecting a
modeling problem. Set `validation.split_ratio > 0` together with
`loop_regularization` for early stopping, exactly as
[`TrainOptions`](../options/TrainOptions.md) describes -- every
nested policy struct here is the identical shared type.

## See Also

- [`TemporalFusionTransformer::learn`](TemporalFusionTransformer/learn.md)
  -- the consumer.
- [`TrainOptions`](../options/TrainOptions.md) -- the generic
  equivalent this mirrors field-for-field.
- [`AutoregressiveTrainOptions`](AutoregressiveTrainOptions.md) -- the
  identical shape for `AutoregressiveForecaster`.
