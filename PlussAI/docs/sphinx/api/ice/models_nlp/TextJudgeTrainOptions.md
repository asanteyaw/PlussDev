# `pluss::ai::ice::models::TextJudgeTrainOptions`

Batch size, learning rate, optimizer, shuffle/seed, and the shared
validation/architecture-and-objective-regularization/execution
policies for [`TextJudge::learn`](TextJudge/learn.md).

## Fields

### `batch_size`

`std::int64_t batch_size = 32`
: How many examples each mini-batch targets.

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
  [`TextJudgeOptions::seed`](TextJudgeOptions.md), which seeds
  parameter initialization instead.

### `validation`

[`ValidationPolicy`](../options/ValidationPolicy.md) `validation = {}`
: Validation split ratio, evaluation cadence, and the monitored
  metric name/direction.

### `architecture_regularization`

[`ArchitectureRegularization`](../options/ArchitectureRegularization.md) `architecture_regularization = {}`
: `dropout > 0.0` overrides
  [`TextJudgeOptions::dropout`](TextJudgeOptions.md) for this
  training run specifically -- the one catalog entry that reads this
  field at all; see
  [`ArchitectureRegularization`'s own Guidance](../options/ArchitectureRegularization.md).

### `objective_regularization`

[`ObjectiveRegularization`](../options/ObjectiveRegularization.md) `objective_regularization = {}`
: Weight decay, L1/L2 penalties, and label smoothing applied to the
  training objective.

### `loop_regularization`

[`LoopRegularization`](../options/LoopRegularization.md) `loop_regularization = {}`
: Early stopping, patience, minimum delta, and best-weight
  restoration.

### `execution`

[`ExecutionPolicy`](../options/ExecutionPolicy.md) `execution = {}`
: Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
  failure, determinism, precision, and platform -- all-defaults,
  unlike the `models_ts` catalog's own `*TrainOptions`, which arrive
  with gradient clipping and non-finite failure pre-wired.

## Guidance

Leave every field at its default for a reasonable first run. There is
no `checkpoint` field here -- unlike `models_ts`'s two
`*TrainOptions`, `TextJudge::learn` does not support mid-training
checkpointing. Set `validation.split_ratio > 0` together with
`loop_regularization` for early stopping. Reach for
`architecture_regularization.dropout` only to override
[`TextJudgeOptions::dropout`](TextJudgeOptions.md) for one particular
training run without reconstructing the model.

## See Also

- [`TextJudge::learn`](TextJudge/learn.md) -- the consumer.
- [`TextJudgeOptions`](TextJudgeOptions.md) -- the model-construction
  counterpart, including the `dropout` this type's own
  `architecture_regularization` can override.
- [`TrainOptions`](../options/TrainOptions.md) -- the generic
  equivalent most of these fields mirror.
