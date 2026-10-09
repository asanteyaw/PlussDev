# `pluss::ai::ice::models::AutoregressiveForecaster` Introduction

## What `AutoregressiveForecaster` Is

`AutoregressiveForecaster` is ice's DeepAR-style probabilistic
forecaster: static covariates (if any) seed an encoder/decoder RNN's
initial state, the encoder consumes the lookback window
(`encoder_inputs` plus the target's own history, `encoder_target`),
and the decoder steps forward through the forecast horizon
(`decoder_inputs`) producing a per-step Gaussian
[`mean`/`scale`](../GaussianForecast.md) rather than a single point
forecast. It is the simpler of the two `models_ts` catalog entries --
[`TemporalFusionTransformer`](../TemporalFusionTransformer.md) adds
variable selection, static-conditioned gating, attention, and
quantile (not Gaussian) output on top of the same encoder/decoder
shape.

## Lifecycle

A model is constructed once from
[`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)
(fixing every shape/hyperparameter) and starts with
[`ready()`](ready.md) `false`. [`learn`](learn.md) trains it in place;
from then on [`predict`](predict.md)/[`assess`](assess.md)/
[`save`](save.md) are all available. [`load`](load.md) is the other
way to reach a `ready()` model, reconstructing both the architecture
and the weights from a saved artifact with no call to `learn` at all.
[`to`](to.md) moves parameters between platforms at any point in this
lifecycle without affecting `ready()`.

## Three Ways In

Every `learn`/`predict`/`assess` overload accepts the model's own
primitive batch type, [`AutoregressiveBatch`](../AutoregressiveBatch.md),
directly -- or a [`data::Batch`](../../data/Batch.md)/
[`data::TimeSeriesBatch`](../../data/TimeSeriesBatch.md), converted
internally with no behavior difference (proven directly in each
method page's own examples, e.g.
[`learn`'s Example 3](learn.md#example-3)). Building an
`AutoregressiveForecaster` straight from a
[`data::TimeSeriesDataSet`](../../data/TimeSeriesDataSet.md) --
`AutoregressiveForecasterOptions::from_dataset(dataset)` for sizing,
then `model.learn(dataset.batch(), ...)` -- is the usual end-to-end
path; see any method page's own examples for the full pattern.

## Related Pages

- [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)
  -- the constructor's configuration, with `from_data`/`from_dataset`
  auto-sizing.
- [`AutoregressiveBatch`](../AutoregressiveBatch.md) -- the primitive
  batch type every overload reduces to.
- [`AutoregressiveTrainOptions`](../AutoregressiveTrainOptions.md) --
  every [`learn`](learn.md) knob.
- [`GaussianForecast`](../GaussianForecast.md) -- what
  [`predict`](predict.md) returns.
- [`TemporalFusionTransformer`](../TemporalFusionTransformer.md) --
  the richer, quantile-output `models_ts` entry built on the same
  encoder/decoder shape.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
