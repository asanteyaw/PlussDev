<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 96e48c08163b -->
# `pluss::ai::ice::models::AutoregressiveForecaster::predict`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::models::GaussianForecast pluss::ai::ice::models::AutoregressiveForecaster::predict(
    const AutoregressiveBatch& batch) const;
[[nodiscard]] pluss::ai::ice::models::GaussianForecast pluss::ai::ice::models::AutoregressiveForecaster::predict(
    const pluss::ai::ice::data::Batch& batch) const;
[[nodiscard]] pluss::ai::ice::models::GaussianForecast pluss::ai::ice::models::AutoregressiveForecaster::predict(
    const pluss::ai::ice::data::TimeSeriesBatch& batch) const;
```

## What It Does

Runs the model forward in evaluation mode (dropout disabled) and
returns a per-horizon-step [`GaussianForecast`](../GaussianForecast.md)
mean/scale. `batch.target`/`encoder_target` are not read for the
forecast itself -- only `static_inputs`/`encoder_inputs`/
`decoder_inputs` feed the prediction (see Example 1). The two
`data::Batch`/`data::TimeSeriesBatch` overloads are convenience
wrappers that construct an [`AutoregressiveBatch`](../AutoregressiveBatch.md)
and forward to the native overload, with no behavior difference (see
Example 2).

## Parameters

`const AutoregressiveBatch& batch` / `const data::Batch& batch` / `const data::TimeSeriesBatch& batch`
: The batch to forecast for, in whichever of the three shapes is
  convenient.

## Returns

[`GaussianForecast`](../GaussianForecast.md) -- `mean`/`scale`, each
shaped `[N, decoder_length]`. Deterministic for a fixed trained model
and a fixed input batch, including when `dropout > 0` (dropout only
applies during [`learn`](learn.md), never at inference time -- see
Example 3).

## Errors

- Throws if `batch`'s tensor shapes disagree with
  [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md).

## Practical Examples

Three examples: confirming the forecast does not depend on
`batch.target` at all, the `data::Batch` overload producing a
byte-identical result to the native overload, and inference-time
determinism holding even with `dropout > 0` at construction.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <random>
#include <vector>

namespace {
namespace ice = pluss::ai::ice;
namespace data = ice::data;
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

data::TimeSeriesBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                      std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> base_dist(-1.0, 1.0);
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> encoder_target_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double base = base_dist(rng);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
      encoder_target_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
    }
  }
  data::TimeSeriesBatch out{};
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.encoder_target = pluss::make_series(encoder_target_flat, {n, encoder_length}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train_ts = make_synthetic(64, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(models::AutoregressiveBatch(train_ts), train_options);

  const auto forecast = model.predict(models::AutoregressiveBatch(train_ts));
  std::cout << "mean.sizes() = [" << forecast.mean.sizes()[0] << ", " << forecast.mean.sizes()[1] << "]\n";

  // predict() never reads `target` -- swapping it for unrelated garbage
  // must not change the forecast at all.
  models::AutoregressiveBatch with_garbage_target(train_ts);
  with_garbage_target.target = pluss::series::randn(with_garbage_target.target.sizes(), f32(), 99ULL);
  const auto forecast_garbage_target = model.predict(with_garbage_target);
  const double diff = forecast.mean.sub(forecast_garbage_target.mean).abs().amax().item<double>();
  std::cout << "predict() ignores target, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
mean.sizes() = [64, 3]
predict() ignores target, diff = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <random>
#include <vector>

namespace {
namespace ice = pluss::ai::ice;
namespace data = ice::data;
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

data::TimeSeriesBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                      std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> base_dist(-1.0, 1.0);
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> encoder_target_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double base = base_dist(rng);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
      encoder_target_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
    }
  }
  data::TimeSeriesBatch out{};
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.encoder_target = pluss::make_series(encoder_target_flat, {n, encoder_length}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train_ts = make_synthetic(64, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(models::AutoregressiveBatch(train_ts), train_options);

  const auto forecast_native = model.predict(models::AutoregressiveBatch(train_ts));
  // The data::Batch overload is exactly AutoregressiveBatch(batch) first,
  // then the native overload.
  const data::Batch core = dataset.data().batch();
  const auto forecast_core = model.predict(core);
  const double diff = forecast_native.mean.sub(forecast_core.mean).abs().amax().item<double>();
  std::cout << "data::Batch overload matches native overload, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
data::Batch overload matches native overload, diff = 0
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <random>
#include <vector>

namespace {
namespace ice = pluss::ai::ice;
namespace data = ice::data;
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

data::TimeSeriesBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                      std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> base_dist(-1.0, 1.0);
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> encoder_target_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double base = base_dist(rng);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
      encoder_target_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(base);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(base);
    }
  }
  data::TimeSeriesBatch out{};
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.encoder_target = pluss::make_series(encoder_target_flat, {n, encoder_length}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train_ts = make_synthetic(64, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.1; // nonzero dropout -- determinism still must hold at inference time.
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(train_ts, train_options);

  const auto first = model.predict(train_ts);
  const auto second = model.predict(train_ts);
  const double diff = first.mean.sub(second.mean).abs().amax().item<double>();
  std::cout << "repeated predict() on a trained model is deterministic, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
repeated predict() on a trained model is deterministic, diff = 0
```

## See Also

- [`GaussianForecast`](../GaussianForecast.md) -- the returned type.
- [`learn`](learn.md) -- trains the model this call reads.
- [`assess`](assess.md) -- a scalar loss instead of the full
  forecast, for a batch that does have a real `target`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
