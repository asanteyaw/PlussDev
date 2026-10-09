# `pluss::ai::ice::models::TextJudge` Introduction

## What `TextJudge` Is

`TextJudge` is a text classifier built on `nlp` `Doc`s/`msharp`
ranges -- the heaviest input-type polymorphism in the catalog.
`learn`/`predict`/`assess` each have four overloads
(`Doc`/`RangeView<std::string_view>`/`std::vector<std::string>`/
`pluss::Note`) accepting the same corpus in whichever shape is
convenient, and all four always agree on the result for the same
underlying texts (proved directly in each method page's own
examples).

## Lifecycle

A model is constructed once from
[`TextJudgeOptions`](../TextJudgeOptions.md) (hidden width,
construction-time dropout, tokenization toggles) and starts with
[`ready()`](ready.md) `false`. [`learn`](learn.md) trains it in
place, fitting [`labels()`](labels.md)'s own vocabulary from whatever
distinct label values it sees (at least two are required); from then
on [`predict`](predict.md)/[`assess`](assess.md)/
[`save`](save.md) are all available. [`load`](load.md) is the other
way to reach a `ready()` model, reconstructing the options, the
label vocabulary, and the trained classifier state from a single
saved artifact, with no call to `learn` at all.

## `to` Is Weaker Here Than In `models_ts`

Unlike [`AutoregressiveForecaster::to`](../../models_ts/AutoregressiveForecaster/to.md)/[`TemporalFusionTransformer::to`](../../models_ts/TemporalFusionTransformer/to.md),
[`TextJudge::to`](to.md) does not move an already-trained model's
parameters to a new device in place -- it only updates
[`TextJudgeOptions::cfg`](../TextJudgeOptions.md)'s platform for a
*future* [`save`](save.md)/[`load`](load.md) round trip to
reconstruct on. A trained model's own predictions are completely
unaffected by calling `to` (see
[`to`'s own Example 1](to.md#example-1)) -- see
[`to`'s own What It Does](to.md) for exactly why.

## No Checkpointing, No `from_data`/`from_dataset`

Unlike every `models_ts` catalog entry,
[`TextJudgeTrainOptions`](../TextJudgeTrainOptions.md) has no
`checkpoint` field -- `learn` does not support mid-training
checkpointing. [`TextJudgeOptions`](../TextJudgeOptions.md) also has
no `from_data`/`from_dataset` factory: `TextJudge` sizes itself from
whatever text it sees at `learn()` time, not from a pre-built
dataset.

## Related Pages

- [`TextJudgeOptions`](../TextJudgeOptions.md) -- the constructor's
  configuration, including the construction-time `dropout`
  [`TextJudgeTrainOptions`](../TextJudgeTrainOptions.md) can override.
- [`TextJudgePrediction`](../TextJudgePrediction.md) -- what
  [`predict`](predict.md) returns per input.
- [`TextJudgeReport`](../TextJudgeReport.md) -- what
  [`assess`](assess.md) returns.
- [`models_ts`](../../models_ts.md) -- the other `models_*` family,
  whose two catalog entries share `TextJudge`'s overall calling
  convention but not its `to`/save-format specifics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
