# `pluss::ai::ice::models::TemporalFusionTransformer`

Multi-horizon quantile forecasting per Lim et al. 2019: variable
selection networks, gated residual networks, an LSTM encoder/decoder,
interpretable causal self-attention.

## Signature

```cpp
explicit pluss::ai::ice::models::TemporalFusionTransformer::TemporalFusionTransformer(
    pluss::ai::ice::models::TFTOptions options);
```

## Construction

```cpp
namespace ice = pluss::ai::ice;
namespace data = ice::data;
namespace models = ice::models;

const auto dataset = data::TimeSeriesDataSet::from_series(my_batch);
auto options = models::TFTOptions::from_dataset(dataset);
options.encoder_length = 10;
options.decoder_length = 5;
options.hidden_size = 32;
options.quantiles = {0.1, 0.5, 0.9};

models::TemporalFusionTransformer model(options);
```

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `assess(batch)` | `double` | [`assess`](TemporalFusionTransformer/assess.md) |
| `learn(train, options = {})` / `learn(train, val, options = {})` | `void` | [`learn`](TemporalFusionTransformer/learn.md) |
| `load(filepath)` | `TemporalFusionTransformer` | [`load`](TemporalFusionTransformer/load.md) |
| `predict(batch)` | `TFTForecast` | [`predict`](TemporalFusionTransformer/predict.md) |
| `ready()` | `bool` | [`ready`](TemporalFusionTransformer/ready.md) |
| `save(filepath)` | `void` | [`save`](TemporalFusionTransformer/save.md) |
| `to(platform)` | `TemporalFusionTransformer&` | [`to`](TemporalFusionTransformer/to.md) |

## Changelog

- **0.1** -- `TemporalFusionTransformer`: `learn`/`predict`/`assess`
  over `TFTBatch`/`data::Batch`/`data::TimeSeriesBatch`, `save`/
  `load`, `ready`, `to`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TemporalFusionTransformer/Introduction
TemporalFusionTransformer/assess
TemporalFusionTransformer/learn
TemporalFusionTransformer/load
TemporalFusionTransformer/predict
TemporalFusionTransformer/ready
TemporalFusionTransformer/save
TemporalFusionTransformer/to
TFTOptions
TFTTrainOptions
```
<!-- apiref:toctree:end -->
