# `ice::models_ts` Introduction

## What This Category Owns

`models_ts` owns ice's time-series forecasting catalog: two complete,
ready-to-train architectures --
[`AutoregressiveForecaster`](AutoregressiveForecaster.md) and
[`TemporalFusionTransformer`](TemporalFusionTransformer.md) -- plus
each one's own configuration (`*Options`), batch shape (`*Batch`),
training knobs (`*TrainOptions`), and forecast result type. It is
ice's first `models_*` family, and the one every later family's own
`*Options::from_data`/`from_dataset` pattern was established by.

## Why Two Entries

[`AutoregressiveForecaster`](AutoregressiveForecaster.md) is the
simpler DeepAR-style baseline: an encoder/decoder RNN with a single
Gaussian output head, and strict batch-shape validation, including
lookback/horizon length.
[`TemporalFusionTransformer`](TemporalFusionTransformer.md) is the
richer architecture from Lim et al. 2019: variable selection,
static-conditioned gating throughout, an LSTM encoder/decoder, causal
self-attention, and quantile (not Gaussian) output -- validated just
as strictly on feature widths, but deliberately permissive about
lookback/horizon length (see
[its own Introduction](TemporalFusionTransformer/Introduction.md#lookback-and-horizon-length-is-looser-than-autoregressiveforecaster)).
Both share the exact same calling convention -- `*Options` (by hand
or auto-sized via `from_data`/`from_dataset`), construct, `learn`,
then `predict`/`assess` -- so switching between them is a type
substitution, not a different workflow.

## Reading Order

1. [`AutoregressiveForecasterOptions`](AutoregressiveForecasterOptions.md) /
   [`AutoregressiveBatch`](AutoregressiveBatch.md) /
   [`AutoregressiveTrainOptions`](AutoregressiveTrainOptions.md) /
   [`GaussianForecast`](GaussianForecast.md) -- the small supporting
   types.
2. [`AutoregressiveForecaster`](AutoregressiveForecaster.md) -- the
   model itself, with the full method table.
3. [`TFTOptions`](TFTOptions.md) / [`TFTBatch`](TFTBatch.md) /
   [`TFTTrainOptions`](TFTTrainOptions.md) /
   [`TFTForecast`](TFTForecast.md) -- the analogous supporting types
   for TFT.
4. [`TemporalFusionTransformer`](TemporalFusionTransformer.md) -- the
   richer model, and where it genuinely differs from entry 2.

## Related Pages

- [`data`](../data.md) -- `data::DataSet`/`data::TimeSeriesDataSet`,
  what both catalog entries' `Options::from_data`/`from_dataset`
  read.
- [`source`](../source.md) -- `Source`/`BatchPlan`, one layer further
  back.
- [`options`](../options.md) -- the nested policy structs every
  `*TrainOptions` here shares with
  [`pluss::ai::ice::TrainOptions`](../options/TrainOptions.md).
