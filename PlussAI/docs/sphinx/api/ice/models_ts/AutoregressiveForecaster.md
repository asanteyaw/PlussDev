# `pluss::ai::ice::models::AutoregressiveForecaster`

A DeepAR-style probabilistic autoregressive forecaster: static
covariates, an encoder/decoder RNN, and a Gaussian output head.

## Signature

```cpp
explicit pluss::ai::ice::models::AutoregressiveForecaster::AutoregressiveForecaster(
    pluss::ai::ice::models::AutoregressiveForecasterOptions options);
```

## Construction

```cpp
namespace ice = pluss::ai::ice;
namespace data = ice::data;
namespace models = ice::models;

const auto dataset = data::TimeSeriesDataSet::from_series(my_batch);
auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
options.hidden_size = 32;
options.rnn_layers = 2;

models::AutoregressiveForecaster model(options);
```

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `assess(batch)` | `double` | [`assess`](AutoregressiveForecaster/assess.md) |
| `learn(train, options = {})` / `learn(train, val, options = {})` | `void` | [`learn`](AutoregressiveForecaster/learn.md) |
| `load(filepath)` | `AutoregressiveForecaster` | [`load`](AutoregressiveForecaster/load.md) |
| `predict(batch)` | `GaussianForecast` | [`predict`](AutoregressiveForecaster/predict.md) |
| `ready()` | `bool` | [`ready`](AutoregressiveForecaster/ready.md) |
| `save(filepath)` | `void` | [`save`](AutoregressiveForecaster/save.md) |
| `to(platform)` | `AutoregressiveForecaster&` | [`to`](AutoregressiveForecaster/to.md) |

## Changelog

- **0.1** -- `AutoregressiveForecaster`: `learn`/`predict`/`assess`
  over `AutoregressiveBatch`/`data::Batch`/`data::TimeSeriesBatch`,
  `save`/`load`, `ready`, `to`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

AutoregressiveForecaster/Introduction
AutoregressiveForecaster/assess
AutoregressiveForecaster/learn
AutoregressiveForecaster/load
AutoregressiveForecaster/predict
AutoregressiveForecaster/ready
AutoregressiveForecaster/save
AutoregressiveForecaster/to
AutoregressiveForecasterOptions
AutoregressiveTrainOptions
```
<!-- apiref:toctree:end -->
