# `pluss::ai::ice::TrainOptions`

Central training configuration: batch size, learning rate, optimizer/loss
kind, shuffle/seed, standardize, and the five nested policy structs
(validation, three regularization structs, checkpoint, execution).

The underlying declared type is `Recipe`; `TrainOptions` is a public
`using TrainOptions = Recipe;` alias -- every signature across `ice`
spells it `TrainOptions`, and this page does too. Passed to
[`Model::learn`](../model/Model.md), [`Coach`](../coach/Coach.md)'s
constructor, and every `models_ts`/`models_nlp`/`models_rl` catalog
entry's own `learn()` (through that entry's own, differently-named but
identically-shaped `*TrainOptions`); the default, `TrainOptions{}`, is
Adam at `1e-3` over 60 epochs with gradient clipping and non-finite
failure already on.

## Fields

### `batch_size`

`std::int64_t batch_size = 32`
: How many samples each mini-batch targets, for the two `Series`-pair
  overloads of `Model::learn`. Ignored by the `Source`-taking overload,
  whose batching is instead whatever that `Source`'s own
  [`BatchPlan`](../source/BatchPlan.md) says.

### `learning_rate`

`double learning_rate = 1e-3`
: The optimizer's step size, interpreted by whichever `optimizer` is
  selected -- same meaning as every `nn::optim` algorithm's own
  `learning_rate`.

### `optimizer`

`OptimizerKind optimizer = OptimizerKind::Adam`
: Which `nn::optim` algorithm [`make_optimizer`](../optim/make_optimizer.md)
  constructs: `Adam` (the default), `AdamW`, `SGD`, `RMSProp`, `Adagrad`,
  or `NAdam`. An enum rather than a string -- picking an unsupported
  optimizer is a compile-time typo, not a runtime surprise discovered
  mid-training.

### `loss`

`LossKind loss = LossKind::MSE`
: Which loss `Model`'s default `compute_loss` dispatches to: `MSE`,
  `BCEWithLogits`, or `CrossEntropy`. Irrelevant for a `Model` subclass
  that overrides `compute_loss` itself.

### `shuffle`

`bool shuffle = true`
: Whether each epoch's mini-batches are drawn in a fresh random
  permutation (`true`, the default) or in fixed ascending order
  (`false`). Only affects the two `Series`-pair `learn` overloads.

### `seed`

`std::uint64_t seed = 0`
: The seed `shuffle`'s permutation (and the internal train/validation
  split, when `validation.split_ratio > 0`) is derived from -- the same
  `seed` on the same data always reproduces the same order and split.

### `standardize`

`bool standardize = false`
: Only consulted by `Model::learn`'s `DataFrame` overload:
  per-feature-column standardization (`(x - mean) / std`) before
  training. When `true`, the fitted mean/std are kept on the model
  (persisted through `save()`/`load()`) and reapplied automatically by
  the `DataFrame` overloads of `infer()`/`assess()` -- a caller never
  recomputes them by hand.

### `validation`

[`ValidationPolicy`](ValidationPolicy.md) `validation = {}`
: Validation split ratio, evaluation cadence, and the monitored metric
  name/direction -- shared with every `models_*` catalog entry's own
  `*TrainOptions`.

### `architecture_regularization`

[`ArchitectureRegularization`](ArchitectureRegularization.md) `architecture_regularization = {}`
: Dropout strength. Shared with `TextJudgeTrainOptions`; the other
  catalog entries bake dropout directly into their own model-construction
  `Options` instead (`TFTOptions::dropout`,
  `AutoregressiveForecasterOptions::dropout`).

### `objective_regularization`

[`ObjectiveRegularization`](ObjectiveRegularization.md) `objective_regularization = {}`
: Weight decay, L1/L2 penalties, and label smoothing applied to the
  training objective -- shared with every `models_*` catalog entry's own
  `*TrainOptions`.

### `loop_regularization`

[`LoopRegularization`](LoopRegularization.md) `loop_regularization = {}`
: Early stopping, patience, minimum delta, and best-weight restoration --
  shared with every `models_*` catalog entry's own `*TrainOptions`.

### `checkpoint`

[`CheckpointPolicy`](CheckpointPolicy.md) `checkpoint = {}`
: Whether/when to write checkpoints during training, where, and how to
  resume -- shared with `models_ts`'s two `*TrainOptions`
  (`models_nlp`/`models_rl` do not use it).

### `execution`

[`ExecutionPolicy`](ExecutionPolicy.md) `execution = {}`
: Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
  failure, determinism, precision, and platform -- shared with every
  `models_*` catalog entry's own `*TrainOptions`.

## Guidance

Leave every field at its default for a reasonable first training run:
Adam at `1e-3`, shuffled mini-batches, gradient clipping and non-finite
failure already on (`execution`'s own defaults), no validation split.
Set `validation.split_ratio > 0` and tune `loop_regularization` together
to get early stopping; they are read together (`EarlyStopCallback` is
constructed from both). Set `standardize = true` only when training
through the `DataFrame` overload with features on very different scales
-- it has no effect on the `Series`-pair overloads, which take tensors a
caller already prepared. Reach for a `models_*` catalog entry's own
`*TrainOptions` instead of this type when training that catalog entry
specifically; the two are separate types that happen to share most
field names and every nested policy struct, not the same type reused.

## See Also

- [`Model::learn`](../model/Model.md) -- the primary consumer.
- [`Coach`](../coach/Coach.md) -- the model-external orchestration
  surface, constructed from a `TrainOptions` directly.
- [`make_optimizer`](../optim/make_optimizer.md) -- builds the optimizer
  `optimizer`/`learning_rate`/`execution` select, standalone.
