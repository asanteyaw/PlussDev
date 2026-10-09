# `ice::options` Introduction

## What This Category Owns

`options` owns `ice`'s central training configuration surface:
[`TrainOptions`](TrainOptions.md) (public alias for the declared
`Recipe` struct), the five nested policy structs it aggregates
([`ValidationPolicy`](ValidationPolicy.md),
[`ArchitectureRegularization`](ArchitectureRegularization.md),
[`ObjectiveRegularization`](ObjectiveRegularization.md),
[`LoopRegularization`](LoopRegularization.md),
[`CheckpointPolicy`](CheckpointPolicy.md),
[`ExecutionPolicy`](ExecutionPolicy.md)), and
[`ProgressPolicy`](ProgressPolicy.md), and
[`AssessReport`](AssessReport.md), the result `assess()` returns.

## The Shared Contract

Every one of the five nested policy structs -- except `CheckpointPolicy`,
which `models_nlp`/`models_rl` don't use -- is reused verbatim across
`TrainOptions` and every `models_*` catalog entry's own, separately
declared `*TrainOptions` (`TFTTrainOptions`,
`AutoregressiveTrainOptions`, `TextJudgeTrainOptions`,
`ActionValueTrainOptions`). Reading one policy struct's page here covers
its fields everywhere it appears -- a catalog entry's own `*TrainOptions`
page lists which of these it includes and cross-links back here instead
of re-explaining each field.

Two different mechanisms read `ObjectiveRegularization`:
`weight_decay`/`decoupled_weight_decay` fold into the optimizer's own
construction (via [`make_optimizer`](../optim/make_optimizer.md)); `l1_penalty`/
`l2_penalty`/`label_smoothing` are applied directly to the computed loss
(via [`Model::compute_loss`](../model/Model.md)'s context-aware
overload). `ValidationPolicy`/`LoopRegularization` are read together by
[`EarlyStopCallback`](../callback/EarlyStopCallback.md); `CheckpointPolicy`
is read entirely by [`Coach::learn`](../coach/Coach.md).

## Reading Order

1. [`TrainOptions`](TrainOptions.md) -- the central type; its own page
   explains what each nested policy struct is for before sending a
   reader to that struct's own page for the field-level detail.
2. Any one nested policy struct, in whatever order matches the training
   behavior being configured -- each is independent of the others.
3. [`AssessReport`](AssessReport.md) -- the much smaller result type
   `assess()` returns, unrelated to configuration.

## Related Pages

- [`model`](../model.md) -- `Model::learn`/`infer`/`assess` are
  `TrainOptions`'s primary consumers.
- [`coach`](../coach.md) -- the model-external orchestration surface
  `TrainOptions` also configures.
- [`optim`](../optim.md) -- `make_optimizer`, built from
  `TrainOptions::optimizer`/`learning_rate`/`objective_regularization`/
  `execution`.
- [`callback`](../callback.md) -- `EarlyStopCallback`, built from
  `validation`/`loop_regularization`.
