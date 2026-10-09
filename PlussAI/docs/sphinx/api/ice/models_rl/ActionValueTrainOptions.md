# `pluss::ai::ice::models::ActionValueTrainOptions`

Batch size, learning rate, optimizer, shuffle/seed, the Huber-vs-MSE
TD-error loss toggle, and the shared validation/regularization/
execution policies for
[`ActionValueAgent::learn`](ActionValueAgent/learn.md).

## Fields

### `batch_size`

`std::int64_t batch_size = 64`
: How many transitions each mini-batch targets.

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

### `use_huber`

`bool use_huber = true`
: Use Huber loss for the TD error (robust to reward/Q-value
  outliers) rather than plain MSE. Also the default for
  [`ActionValueAgent::assess`](ActionValueAgent/assess.md)'s own
  `use_huber` parameter.

### `seed`

`std::uint64_t seed = 0`
: Seeds `shuffle`'s permutation and the internal train/validation
  split (when `validation.split_ratio > 0`) -- independent of
  [`ActionValueAgentOptions::seed`](ActionValueAgentOptions.md),
  which seeds parameter initialization instead.

### `progress`

[`ProgressPolicy`](../options/ProgressPolicy.md) `progress = {}`
: Live completed-epoch reporting. The default `loss` is the row-weighted mean
  Huber or MSE TD loss for the epoch; `val_loss` appears on validation epochs.
  Add `"mean_q"` to `metrics` to report the validation batch's mean selected Q
  value as well. Silent by default.

### `validation`

[`ValidationPolicy`](../options/ValidationPolicy.md) `validation = {}`
: Validation split ratio, evaluation cadence, and the monitored
  metric name/direction.

### `objective_regularization`

[`ObjectiveRegularization`](../options/ObjectiveRegularization.md) `objective_regularization = {}`
: Weight decay, L1/L2 penalties, and label smoothing applied to the
  TD-error objective.

### `loop_regularization`

[`LoopRegularization`](../options/LoopRegularization.md) `loop_regularization = {}`
: Early stopping, patience, minimum delta, and best-weight
  restoration.

### `execution`

[`ExecutionPolicy`](../options/ExecutionPolicy.md) `execution = {}`
: Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
  failure, determinism, precision, and platform -- all-defaults, like
  [`TextJudgeTrainOptions`](../models_nlp/TextJudgeTrainOptions.md),
  not pre-wired with gradient clipping the way `models_ts`'s own
  `*TrainOptions` are.

## Guidance

Leave every field at its default for a reasonable first run. There is
no `checkpoint` field -- like `TextJudge`, `ActionValueAgent::learn`
does not support mid-training checkpointing. Set
`validation.split_ratio > 0` together with `loop_regularization` for
early stopping; the validation metric here is
[`ActionValueReport::loss`](ActionValueReport.md) under whichever of
`use_huber`'s two losses this run selected.

## See Also

- [`ActionValueAgent::learn`](ActionValueAgent/learn.md) -- the
  consumer.
- [`ActionValueAgentOptions`](ActionValueAgentOptions.md) -- the
  model-construction counterpart.
- [`TrainOptions`](../options/TrainOptions.md) -- the generic
  equivalent most of these fields mirror.
