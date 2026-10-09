# `pluss::ai::ice::models::TFTForecast`

The per-quantile forecast plus interpretable attention weights
[`TemporalFusionTransformer::predict`](TemporalFusionTransformer/predict.md)
returns.

## Signature

```cpp
pluss::ai::ice::models::TFTForecast::TFTForecast() = default;
```

A plain aggregate -- `quantiles` and `attention_weights` are both
undefined by default.

## State

### `quantiles`

`Series quantiles = pluss::series::undefined()`
: `[N, decoder_length, num_quantiles]` -- the last axis is ordered
  exactly as [`TFTOptions::quantiles`](TFTOptions.md) was given (the
  default `{0.1, 0.5, 0.9}` puts the median at index `1`).

### `attention_weights`

`Series attention_weights = pluss::series::undefined()`
: `[N, decoder_length, encoder_length + decoder_length]` -- the
  causal self-attention layer's own weights, a softmax output: every
  row along the last axis sums to `1` (see Example 3).

## Methods

None -- `TFTForecast` is a plain read-only result value; see
`## State` above. Slice out one quantile (e.g. the median) with
ordinary `Series` indexing, as every `TemporalFusionTransformer`
example does.

## Practical Examples

Three examples: the shapes `predict()` actually returns, building one
by hand from raw tensors, and confirming `attention_weights`' rows
are a genuine softmax distribution.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <random>
#include <vector>

namespace {
namespace ice = pluss::ai::ice;
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }
} // namespace

int main() {
  constexpr std::int64_t kN = 48;
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;

  std::mt19937_64 rng(5);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(kN));
  std::vector<float> encoder_flat(static_cast<std::size_t>(kN * kEncoderLength));
  std::vector<float> decoder_flat(static_cast<std::size_t>(kN * kDecoderLength));
  std::vector<float> target_flat(static_cast<std::size_t>(kN * kDecoderLength));
  for (std::int64_t i = 0; i < kN; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < kEncoderLength; ++t) {
      encoder_flat[static_cast<std::size_t>(i * kEncoderLength + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < kDecoderLength; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(kDecoderLength);
      decoder_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(level + 0.5 * position);
    }
  }

  models::TFTBatch batch{};
  batch.static_inputs = pluss::make_series(static_flat, {kN, 1}, f32());
  batch.encoder_inputs = pluss::make_series(encoder_flat, {kN, kEncoderLength, 1}, f32());
  batch.decoder_inputs = pluss::make_series(decoder_flat, {kN, kDecoderLength, 1}, f32());
  batch.target = pluss::make_series(target_flat, {kN, kDecoderLength}, f32());

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.quantiles = {0.1, 0.5, 0.9};
  options.cfg = f32();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  model.learn(batch, train_options);

  const auto forecast = model.predict(batch);
  std::cout << "quantiles.sizes() = [" << forecast.quantiles.sizes()[0] << ", " << forecast.quantiles.sizes()[1]
             << ", " << forecast.quantiles.sizes()[2] << "]\n";
  std::cout << "attention_weights.sizes() = [" << forecast.attention_weights.sizes()[0] << ", "
             << forecast.attention_weights.sizes()[1] << ", " << forecast.attention_weights.sizes()[2] << "]\n";

  return 0;
}
```

Output

```text
quantiles.sizes() = [48, 3, 3]
attention_weights.sizes() = [48, 3, 9]
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
  models::TFTForecast forecast{};
  forecast.quantiles = pluss::series::randn({2, 3, 3}, f32, 1ULL);
  forecast.attention_weights = pluss::series::randn({2, 3, 9}, f32, 2ULL);

  std::cout << "quantiles.defined() = " << forecast.quantiles.defined() << "\n";
  std::cout << "attention_weights.defined() = " << forecast.attention_weights.defined() << "\n";

  return 0;
}
```

Output

```text
quantiles.defined() = 1
attention_weights.defined() = 1
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
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }
} // namespace

int main() {
  constexpr std::int64_t kN = 48;
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;

  std::mt19937_64 rng(5);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(kN));
  std::vector<float> encoder_flat(static_cast<std::size_t>(kN * kEncoderLength));
  std::vector<float> decoder_flat(static_cast<std::size_t>(kN * kDecoderLength));
  std::vector<float> target_flat(static_cast<std::size_t>(kN * kDecoderLength));
  for (std::int64_t i = 0; i < kN; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < kEncoderLength; ++t) {
      encoder_flat[static_cast<std::size_t>(i * kEncoderLength + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < kDecoderLength; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(kDecoderLength);
      decoder_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * kDecoderLength + t)] = static_cast<float>(level + 0.5 * position);
    }
  }

  models::TFTBatch batch{};
  batch.static_inputs = pluss::make_series(static_flat, {kN, 1}, f32());
  batch.encoder_inputs = pluss::make_series(encoder_flat, {kN, kEncoderLength, 1}, f32());
  batch.decoder_inputs = pluss::make_series(decoder_flat, {kN, kDecoderLength, 1}, f32());
  batch.target = pluss::make_series(target_flat, {kN, kDecoderLength}, f32());

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = kEncoderLength;
  options.decoder_length = kDecoderLength;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.quantiles = {0.1, 0.5, 0.9};
  options.cfg = f32();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(batch, train_options);

  const auto forecast = model.predict(batch);
  // attention_weights is a softmax output -- every row over the
  // (encoder+decoder) axis must sum to 1.
  const double row_sum_error = forecast.attention_weights.sum(-1, false).sub(1.0).abs().amax().item<double>();
  std::cout << "attention_weights rows sum to ~1, max error = " << row_sum_error << "\n";

  return 0;
}
```

Output

```text
attention_weights rows sum to ~1, max error = 1.19209e-07
```

## Changelog

- **0.1** -- `TFTForecast`: `quantiles`/`attention_weights`.

## See Also

- [`TemporalFusionTransformer::predict`](TemporalFusionTransformer/predict.md)
  -- the only producer.
- [`TFTOptions::quantiles`](TFTOptions.md) -- what orders the last
  axis of `quantiles`.
- [`GaussianForecast`](GaussianForecast.md) -- `AutoregressiveForecaster`'s
  own Gaussian-shaped (not quantile) forecast result.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
