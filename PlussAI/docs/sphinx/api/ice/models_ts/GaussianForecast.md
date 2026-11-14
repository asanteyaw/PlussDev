# `pluss::ai::ice::models::GaussianForecast`

The per-horizon-step Gaussian mean/scale
[`AutoregressiveForecaster::predict`](AutoregressiveForecaster/predict.md)
returns -- DeepAR-style probabilistic, not a point forecast.

## Signature

```cpp
pluss::ai::ice::models::GaussianForecast::GaussianForecast() = default;
```

A plain aggregate -- `mean` and `scale` are both undefined by
default.

## State

### `mean`

`Series mean = pluss::series::undefined()`
: `[N, decoder_length]` -- the predicted Gaussian mean at each
  forecast-horizon step.

### `scale`

`Series scale = pluss::series::undefined()`
: `[N, decoder_length]`, matching `mean`'s own shape exactly --
  the predicted Gaussian standard deviation, always strictly
  positive.

## Methods

None -- `GaussianForecast` is a plain read-only result value; see
`## State` above. Build a confidence interval from it with ordinary
`Series` arithmetic, as Example 3 below does.

## Practical Examples

Three examples: the shapes `predict()` actually returns, building one
by hand from raw tensors, and deriving a confidence interval from
`mean`/`scale` with plain `Series` arithmetic.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <random>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  constexpr std::int64_t kN = 64;
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;

  std::mt19937_64 rng(11);
  std::uniform_real_distribution<double> base_dist(-1.0, 1.0);
  std::vector<float> encoder_flat(static_cast<std::size_t>(kN * kEncoderLength));
  std::vector<float> decoder_flat(static_cast<std::size_t>(kN * kDecoderLength));
  std::vector<float> encoder_target_flat(static_cast<std::size_t>(kN * kEncoderLength));
  std::vector<float> target_flat(static_cast<std::size_t>(kN * kDecoderLength));
  for (std::int64_t i = 0; i < kN; ++i) {
    const double base = base_dist(rng);
    for (std::int64_t t = 0; t < kEncoderLength; ++t) {
      encoder_flat[static_cast<std::size_t>(i * kEncoderLength + t)] = static_cast<float>(base);
      encoder_target_flat[static_cast<std::size_t>(i * kEncoderLength + t)] = static_cast<float>(base);
    }
    for (std::int64_t t = 0; t < kDecoderLength; ++t) {
      decoder_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(base);
      target_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(base);
    }
  }

  data::TimeSeriesBatch ts_batch{};
  ts_batch.encoder_inputs = pluss::make_series(encoder_flat, {kN, kEncoderLength, 1}, f32);
  ts_batch.decoder_inputs = pluss::make_series(decoder_flat, {kN, kDecoderLength, 1}, f32);
  ts_batch.encoder_target = pluss::make_series(encoder_target_flat, {kN, kEncoderLength}, f32);
  ts_batch.target = pluss::make_series(target_flat, {kN, kDecoderLength}, f32);

  const auto dataset = data::TimeSeriesDataSet::from_series(ts_batch);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32;

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 30;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(dataset.batch(), train_options);

  const auto forecast = model.predict(ts_batch);
  std::cout << "mean.sizes() = [" << forecast.mean.sizes()[0] << ", " << forecast.mean.sizes()[1] << "]\n";
  std::cout << "scale.sizes() == mean.sizes(): " << (forecast.scale.sizes() == forecast.mean.sizes()) << "\n";
  const double min_scale = forecast.scale.amin().item<double>();
  std::cout << "scale is strictly positive: " << (min_scale > 0.0) << "\n";

  return 0;
}
```

Output

```text
mean.sizes() = [64, 3]
scale.sizes() == mean.sizes(): 1
scale is strictly positive: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  models::GaussianForecast forecast{};
  forecast.mean = pluss::make_series({1.0f, 2.0f, 3.0f}, {1, 3}, f32);
  forecast.scale = pluss::make_series({0.1f, 0.2f, 0.3f}, {1, 3}, f32);

  std::cout << "mean.defined() = " << forecast.mean.defined() << "\n";
  std::cout << "scale.defined() = " << forecast.scale.defined() << "\n";

  return 0;
}
```

Output

```text
mean.defined() = 1
scale.defined() = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  models::GaussianForecast forecast{};
  forecast.mean = pluss::make_series({1.0f, 2.0f, 3.0f}, {1, 3}, f32);
  forecast.scale = pluss::make_series({0.1f, 0.2f, 0.3f}, {1, 3}, f32);

  // A 90% interval under the Gaussian: mean +- 1.645 * scale.
  const auto lower = forecast.mean.sub(forecast.scale.mul(1.645));
  const auto upper = forecast.mean.add(forecast.scale.mul(1.645));
  const auto lower_vec = lower.to(pluss::pCPU, ScalarType::Float64).reshape({3}).items<double>();
  const auto upper_vec = upper.to(pluss::pCPU, ScalarType::Float64).reshape({3}).items<double>();
  for (std::size_t t = 0; t < lower_vec.size(); ++t) {
    std::cout << "step " << t << ": [" << lower_vec[t] << ", " << upper_vec[t] << "]\n";
  }

  return 0;
}
```

Output

```text
step 0: [0.8355, 1.1645]
step 1: [1.671, 2.329]
step 2: [2.5065, 3.4935]
```

## Changelog

- **0.1** -- `GaussianForecast`: `mean`/`scale`.

## See Also

- [`AutoregressiveForecaster::predict`](AutoregressiveForecaster/predict.md)
  -- the only producer.
- [`TFTForecast`](TFTForecast.md) -- `TemporalFusionTransformer`'s own
  quantile-shaped (not Gaussian) forecast result.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
