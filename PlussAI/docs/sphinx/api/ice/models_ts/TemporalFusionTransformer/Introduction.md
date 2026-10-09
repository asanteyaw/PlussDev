# `pluss::ai::ice::models::TemporalFusionTransformer` Introduction

## What `TemporalFusionTransformer` Is

`TemporalFusionTransformer` implements Lim et al. 2019's architecture:
variable selection networks pick which static/encoder/decoder
features matter per example, gated residual networks condition
everything on the static context, an LSTM steps through the encoder/
decoder sequence, and a causal self-attention layer over the full
sequence gives interpretable, multi-horizon attention. Unlike
[`AutoregressiveForecaster`](../AutoregressiveForecaster.md)'s single
Gaussian mean/scale, it predicts a configurable set of
[`quantiles`](../TFTOptions.md) directly -- the richer, more
interpretable of the two `models_ts` catalog entries.

Two deliberate scope cuts from the original architecture, documented
rather than silently assumed away: one shared LSTM context (from the
static encoder) is broadcast across every LSTM layer rather than a
distinct per-layer projection, and this implementation's `*_length`
fields are not enforced at inference time -- see the next section.

## Lifecycle

A model is constructed once from [`TFTOptions`](../TFTOptions.md)
(fixing every shape/hyperparameter) and starts with
[`ready()`](ready.md) `false`. [`learn`](learn.md) trains it in place;
from then on [`predict`](predict.md)/[`assess`](assess.md)/
[`save`](save.md) are all available. [`load`](load.md) is the other
way to reach a `ready()` model, reconstructing both the architecture
(including the exact `quantiles` list) and the weights from a saved
artifact with no call to `learn` at all. [`to`](to.md) moves
parameters between platforms at any point in this lifecycle without
affecting `ready()`.

## Lookback And Horizon Length Is Looser Than AutoregressiveForecaster

[`AutoregressiveForecaster`](../AutoregressiveForecaster.md) checks
every batch's tensor shapes -- including the lookback/horizon
*length* axis -- against
[`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)
before `learn`/`predict`/`assess` run. `TemporalFusionTransformer`
checks feature *widths* (`encoder_size`/`decoder_size`/`static_size`)
just as strictly, but deliberately leaves the *length* axis
(`encoder_inputs`/`decoder_inputs`' own `sizes()[1]`) unchecked: a
batch with a different lookback length than
[`TFTOptions::encoder_length`](../TFTOptions.md) still predicts
correctly, since the LSTM and attention layer are architecturally
length-agnostic -- see
[`predict`'s own Example 4](predict.md#example-4). Width mismatches
are not architecturally safe the same way: `VariableSelectionNetwork`
(`tft_blocks.h`) reads exactly `*_size` columns out of its input
regardless of that input's actual width, silently ignoring any extra
columns a too-wide batch has -- which is exactly why the width check
exists and the length check does not; see
[`predict`'s own Example 5](predict.md#example-5).

## Categorical Variables

[`TFTOptions::static_categoricals`/`encoder_categoricals`/`decoder_categoricals`](../TFTOptions.md)
route a marked slot through a real per-category embedding lookup
rather than treating its integer code as an ordinal/continuous
number -- see
[`TFTOptions`'s own Guidance](../TFTOptions.md) and
[`data::CategoricalVariable`](../../data/CategoricalVariable.md).

## Related Pages

- [`TFTOptions`](../TFTOptions.md) -- the constructor's configuration,
  with `from_data`/`from_dataset` auto-sizing.
- [`TFTBatch`](../TFTBatch.md) -- the primitive batch type every
  overload reduces to.
- [`TFTTrainOptions`](../TFTTrainOptions.md) -- every
  [`learn`](learn.md) knob.
- [`TFTForecast`](../TFTForecast.md) -- what
  [`predict`](predict.md) returns.
- [`AutoregressiveForecaster`](../AutoregressiveForecaster.md) -- the
  simpler, Gaussian-output `models_ts` entry built on the same
  encoder/decoder shape, with stricter lookback/horizon-length
  validation (width validation is equally strict on both).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
