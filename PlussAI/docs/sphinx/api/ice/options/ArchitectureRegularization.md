# `pluss::ai::ice::ArchitectureRegularization`

Dropout strength.

Nested in [`TrainOptions::architecture_regularization`](TrainOptions.md)
and [`TextJudgeTrainOptions::architecture_regularization`](../models_nlp/TextJudgeTrainOptions.md).
`Model` itself does not read this field -- dropout lives inside whatever
`nn::` layers a `Model` subclass composes (a `Dropout`-equipped layer's
own construction options), not as a generic knob `Model` applies on a
caller's behalf. `TextJudge` is the one catalog entry that does read it
directly, to override its own model-construction dropout at train time
without reconstructing the model.

## Fields

### `dropout`

`double dropout = 0.0`
: The dropout probability. `0.0` (the default) means "no override" --
  for `TextJudge`, this leaves its own `TextJudgeOptions::dropout`
  (fixed at construction) in effect; for every other catalog entry and
  for a plain `Model` subclass, this field is simply unread. Set it
  `> 0.0` for `TextJudge` specifically to change its effective dropout
  at `learn()` time, separately from the model-construction value.

## Guidance

Leave at the default `0.0` unless training a `TextJudge` and
specifically wanting to override its construction-time dropout for this
training run. For every other model -- the `models_ts` catalog, a plain
`Model`/`Outline` -- set dropout through that model's own construction
options instead (`TFTOptions::dropout`,
`AutoregressiveForecasterOptions::dropout`, a `Dropout` layer's own
options inside an `Outline`'s `Schedule`); this field has no effect
there.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`TextJudgeOptions`](../models_nlp/TextJudgeOptions.md) -- the
  model-construction dropout this field can override at train time.
