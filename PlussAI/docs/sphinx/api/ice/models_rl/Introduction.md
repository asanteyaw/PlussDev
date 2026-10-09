# `ice::models_rl` Introduction

## What This Category Owns

`models_rl` owns ice's reinforcement-learning catalog: one complete,
ready-to-train agent -- [`ActionValueAgent`](ActionValueAgent.md) --
plus its own configuration (`ActionValueAgentOptions`), training
knobs (`ActionValueTrainOptions`), transition batch
(`ActionValueBatch`), and assessment report (`ActionValueReport`).

## How `ActionValueAgent` Differs From `models_ts`/`models_nlp`

`ActionValueAgent` is the only `models_*` catalog entry with no
`data::`-family integration at all -- no `from_data`/`from_dataset`
factory, and
[`ActionValueBatch`](ActionValueBatch.md) has no converting
constructor from a [`data::Batch`](../data/Batch.md). It is also the
only entry with a dedicated single-observation inference method,
[`act`](ActionValueAgent/act.md), alongside the batch-oriented
[`predict`](ActionValueAgent/predict.md) every catalog entry has.
Its [`to`](ActionValueAgent/to.md) genuinely moves live parameters
between platforms, the same real behavior
[`AutoregressiveForecaster::to`](../models_ts/AutoregressiveForecaster/to.md)/[`TemporalFusionTransformer::to`](../models_ts/TemporalFusionTransformer/to.md)
have -- unlike [`TextJudge::to`](../models_nlp/TextJudge/to.md)'s
weaker, save/load-only version.

## Reading Order

1. [`ActionValueAgentOptions`](ActionValueAgentOptions.md) /
   [`ActionValueTrainOptions`](ActionValueTrainOptions.md) -- the
   configuration types.
2. [`ActionValueBatch`](ActionValueBatch.md) /
   [`ActionValueReport`](ActionValueReport.md) -- the transition
   batch and assessment result types.
3. [`ActionValueAgent`](ActionValueAgent.md) -- the agent itself,
   with the full method table.

## Related Pages

- [`models_ts`](../models_ts.md) / [`models_nlp`](../models_nlp.md)
  -- the other two `models_*` families.
- [`options`](../options.md) -- the nested policy structs
  `ActionValueTrainOptions` shares with
  [`TrainOptions`](../options/TrainOptions.md).
