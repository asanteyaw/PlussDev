# `pluss::ai::ice::models` (models_ts)

`models_ts` is ice's time-series forecasting catalog: two ready-made,
"nice, niche" architectures built on the generic
[`data`](data.md)/[`source`](source.md) foundation --
[`AutoregressiveForecaster`](models_ts/AutoregressiveForecaster.md) (a
DeepAR-style probabilistic forecaster) and
[`TemporalFusionTransformer`](models_ts/TemporalFusionTransformer.md)
(Lim et al. 2019's multi-horizon quantile architecture) -- plus each
one's own `Options`/`Batch`/`TrainOptions`/forecast-result types.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`AutoregressiveForecasterOptions`](models_ts/AutoregressiveForecasterOptions.md) | widths/lengths/hyperparameters for `AutoregressiveForecaster`, with `from_data`/`from_dataset` auto-sizing |
| [`AutoregressiveBatch`](models_ts/AutoregressiveBatch.md) | `AutoregressiveForecaster`'s own batch shape |
| [`AutoregressiveTrainOptions`](models_ts/AutoregressiveTrainOptions.md) | batch size/learning rate/optimizer/shared policies for `AutoregressiveForecaster::learn` |
| [`GaussianForecast`](models_ts/GaussianForecast.md) | the Gaussian mean/scale `AutoregressiveForecaster::predict` returns |
| [`AutoregressiveForecaster`](models_ts/AutoregressiveForecaster.md) | the DeepAR-style model itself: `learn`/`predict`/`assess`/`save`/`load`/`ready`/`to` |
| [`TFTOptions`](models_ts/TFTOptions.md) | widths/lengths/hyperparameters/`quantiles` for `TemporalFusionTransformer`, with `from_data`/`from_dataset` auto-sizing |
| [`TFTBatch`](models_ts/TFTBatch.md) | `TemporalFusionTransformer`'s own batch shape |
| [`TFTTrainOptions`](models_ts/TFTTrainOptions.md) | batch size/learning rate/optimizer/shared policies for `TemporalFusionTransformer::learn` |
| [`TFTForecast`](models_ts/TFTForecast.md) | the per-quantile forecast plus attention weights `TemporalFusionTransformer::predict` returns |
| [`TemporalFusionTransformer`](models_ts/TemporalFusionTransformer.md) | the TFT model itself: `learn`/`predict`/`assess`/`save`/`load`/`ready`/`to` |

## How To Call It

Build a forecaster from the dataset's window shapes, train it with verbose logging, and read the forecast's mean and scale. The validation split is off because this example has only four windows.

Build a forecaster from the dataset's window shapes, train it with verbose logging, and read the forecast's mean and scale. The validation split is off because this example has only four windows.

```cpp
#include <pluss/pluss.h>
#include <pluss/mic.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  namespace models = ice::models;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto f32 = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const DataFrame df({
      {"group", Slate(pluss::make_note({"g0", "g0", "g0", "g0", "g1", "g1", "g1", "g1"}))},
      {"region", Slate(pluss::make_note({"north", "north", "north", "north", "south", "south", "south", "south"}))},
      {"t", Slate(pluss::make_series({0.0, 1.0, 2.0, 3.0, 0.0, 1.0, 2.0, 3.0}, f32))},
      {"y", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, f32))},
  });

  data::TimeSeriesDataSetOptions options{};
  options.time_idx = "t";
  options.group_ids = {"group"};
  options.target = "y";
  options.variables = {{"region", data::VariableKind::StaticCategorical}};
  options.encoder_length = 2;
  options.decoder_length = 1;
  options.cfg = f32;

  const auto dataset = data::TimeSeriesDataSet::from_dataframe(df, options);

  auto ar_options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  ar_options.hidden_size = 32;
  models::AutoregressiveForecaster forecaster(ar_options);
  models::AutoregressiveTrainOptions train{};
  train.execution.max_epochs = 3;
  train.progress.verbose = true;
  train.validation.split_ratio = 0.0;
  forecaster.learn(dataset.batch(), train);
  const auto forecast = forecaster.predict(dataset.batch());
  std::cout << "forecast mean elements = " << forecast.mean.numel() << "\n";
  std::cout << "forecast scale elements = " << forecast.scale.numel() << "\n";
  return 0;
}
```

Output

```text
epoch 1/3: loss=36.5711
epoch 2/3: loss=35.6028
epoch 3/3: loss=34.2922
forecast mean elements = 4
forecast scale elements = 4
```

Output

```text
epoch 1/3: loss=36.5711
epoch 2/3: loss=35.6028
epoch 3/3: loss=34.2922
forecast mean elements = 4
forecast scale elements = 4
```

Both catalog entries follow the exact same shape: build `*Options`
(by hand, or auto-sized from a
[`data::DataSet`](data/DataSet.md)/[`data::TimeSeriesDataSet`](data/TimeSeriesDataSet.md)
via `from_data`/`from_dataset`), construct the model, call `learn`
with its own primitive batch type or either `data::Batch`/
`data::TimeSeriesBatch` convenience overload, then `predict`/`assess`.
Both validate every batch's feature widths against their own
`Options`; `AutoregressiveForecaster` additionally validates
lookback/horizon length, while `TemporalFusionTransformer`
deliberately leaves length unchecked (see
[its own Introduction](models_ts/TemporalFusionTransformer/Introduction.md#lookback-and-horizon-length-is-looser-than-autoregressiveforecaster))
-- pick the stricter one when that extra safety matters more than
flexibility.

## Related Pages

- [`data`](data.md) -- `data::DataSet`/`data::TimeSeriesDataSet`,
  what both catalog entries' `Options::from_data`/`from_dataset`
  read and what their `data::Batch`/`data::TimeSeriesBatch` overloads
  convert from.
- [`source`](source.md) -- `Source`/`BatchPlan`, one layer further
  back, behind [`data::DataSet::source`](data/DataSet/source.md).
- [`options`](options.md) -- `ValidationPolicy`/`ObjectiveRegularization`/
  `LoopRegularization`/`CheckpointPolicy`/`ExecutionPolicy`, the
  nested policy structs both `*TrainOptions` types share.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

models_ts/Introduction
models_ts/AutoregressiveForecasterOptions
models_ts/AutoregressiveBatch
models_ts/AutoregressiveTrainOptions
models_ts/GaussianForecast
models_ts/AutoregressiveForecaster
models_ts/TFTOptions
models_ts/TFTBatch
models_ts/TFTTrainOptions
models_ts/TFTForecast
models_ts/TemporalFusionTransformer
```
<!-- apiref:toctree:end -->
