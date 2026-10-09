# `ice::models_nlp` Introduction

## What This Category Owns

`models_nlp` owns ice's text catalog: one complete, ready-to-train
classifier -- [`TextJudge`](TextJudge.md) -- plus its own
configuration (`TextJudgeOptions`), training knobs
(`TextJudgeTrainOptions`), and the two small result types
(`TextJudgePrediction`/`TextJudgeReport`).

## Why `TextJudge` Looks Different From `models_ts`

`TextJudge` follows the same overall shape as every `models_ts`
catalog entry -- `*Options` at construction, `learn`, then
`predict`/`assess` -- but differs in three ways worth knowing before
reaching for it: its `learn`/`predict`/`assess` methods each have
*four* overloads (one per text-input shape) rather than three batch
shapes; its [`save`](TextJudge/save.md) writes a single text artifact
rather than a header-plus-weights pair; and its
[`to`](TextJudge/to.md) does not move an already-trained model's
parameters in place the way
[`AutoregressiveForecaster::to`](../models_ts/AutoregressiveForecaster/to.md)/[`TemporalFusionTransformer::to`](../models_ts/TemporalFusionTransformer/to.md)
do -- see
[`TextJudge`'s own Introduction](TextJudge/Introduction.md) for the
full account of each.

## Reading Order

1. [`TextJudgeOptions`](TextJudgeOptions.md) /
   [`TextJudgeTrainOptions`](TextJudgeTrainOptions.md) -- the
   configuration types, including the `dropout` override one reads
   from the other.
2. [`TextJudgePrediction`](TextJudgePrediction.md) /
   [`TextJudgeReport`](TextJudgeReport.md) -- the two small result
   types.
3. [`TextJudge`](TextJudge.md) -- the classifier itself, with the
   full method table.

## Related Pages

- [`models_ts`](../models_ts.md) -- the other `models_*` family.
- [`options`](../options.md) -- the nested policy structs
  `TextJudgeTrainOptions` shares with
  [`TrainOptions`](../options/TrainOptions.md).
