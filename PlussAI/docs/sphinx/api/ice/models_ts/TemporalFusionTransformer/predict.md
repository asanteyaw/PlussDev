<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6c3bd5a0808b -->
# `pluss::ai::ice::models::TemporalFusionTransformer::predict`

[Back to `pluss::ai::ice::models::TemporalFusionTransformer`](../TemporalFusionTransformer.md)

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::models::TFTForecast pluss::ai::ice::models::TemporalFusionTransformer::predict(
    const TFTBatch& batch) const;
[[nodiscard]] pluss::ai::ice::models::TFTForecast pluss::ai::ice::models::TemporalFusionTransformer::predict(
    const pluss::ai::ice::data::Batch& batch) const;
[[nodiscard]] pluss::ai::ice::models::TFTForecast pluss::ai::ice::models::TemporalFusionTransformer::predict(
    const pluss::ai::ice::data::TimeSeriesBatch& batch) const;
```

## What It Does

Runs the model forward in evaluation mode (dropout disabled) and
returns a [`TFTForecast`](../TFTForecast.md) -- per-quantile forecasts
plus the causal self-attention layer's own weights. `batch.target` is
not read for the forecast itself -- only `static_inputs`/
`encoder_inputs`/`decoder_inputs` feed the prediction (see Example 1).
The two `data::Batch`/`data::TimeSeriesBatch` overloads are
convenience wrappers that construct a [`TFTBatch`](../TFTBatch.md) and
forward to the native overload, with no behavior difference (see
Example 2).

## Parameters

`const TFTBatch& batch` / `const data::Batch& batch` / `const data::TimeSeriesBatch& batch`
: The batch to forecast for, in whichever of the three shapes is
  convenient.

## Returns

[`TFTForecast`](../TFTForecast.md) -- `quantiles` shaped
`[N, decoder_length, num_quantiles]`, `attention_weights` shaped
`[N, decoder_length, encoder_length + decoder_length]`, where
`encoder_length`/`decoder_length` here are `batch`'s own actual
timestep counts, not necessarily
[`TFTOptions::encoder_length`/`decoder_length`](../TFTOptions.md) --
see the note on geometry below.
Deterministic for a fixed trained model and a fixed input batch,
including when `dropout > 0` (dropout only applies during
[`learn`](learn.md), never at inference time -- see Example 3).

## Errors

- Throws if `batch.encoder_inputs`/`decoder_inputs` is undefined, or
  if `batch.static_inputs` is undefined while
  [`TFTOptions::static_size`](../TFTOptions.md) `> 0`.
- Throws if `batch.encoder_inputs`/`decoder_inputs`/`static_inputs`'
  own feature *width* (each tensor's last axis) disagrees with
  [`TFTOptions::encoder_size`/`decoder_size`/`static_size`](../TFTOptions.md)
  -- see Example 5.
- Unlike [`AutoregressiveForecaster::predict`](../AutoregressiveForecaster/predict.md),
  `batch`'s actual lookback/horizon *lengths* are not checked against
  [`TFTOptions::encoder_length`/`decoder_length`](../TFTOptions.md) --
  the LSTM encoder/decoder and the causal self-attention layer both
  adapt to whatever lengths `batch` actually has, so a batch with a
  different lookback window than training still predicts correctly
  (see Example 4). This is the one respect in which
  `TemporalFusionTransformer` stays more permissive than
  `AutoregressiveForecaster`; feature widths are checked just as
  strictly on both.

## Practical Examples

Five examples: confirming the forecast does not depend on
`batch.target` at all, the `data::Batch` overload producing a
byte-identical result to the native overload, inference-time
determinism holding even with `dropout > 0` at construction, `predict`
running correctly against a batch with a different lookback length
than the one the model was trained with, and a batch whose
`encoder_inputs` is wider than `encoder_size` being rejected.

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

models::TFTBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                 std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(level + 0.5 * position);
    }
  }
  models::TFTBatch out{};
  out.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train = make_synthetic(48, 6, 3, 5ULL);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.cfg = f32();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  const auto forecast = model.predict(train);
  std::cout << "quantiles.sizes() = [" << forecast.quantiles.sizes()[0] << ", " << forecast.quantiles.sizes()[1]
             << ", " << forecast.quantiles.sizes()[2] << "]\n";

  // predict() never reads `target` -- swapping it for unrelated garbage
  // must not change the forecast at all.
  models::TFTBatch with_garbage_target = train;
  with_garbage_target.target = pluss::series::randn(with_garbage_target.target.sizes(), f32(), 99ULL);
  const auto forecast_garbage_target = model.predict(with_garbage_target);
  const double diff = forecast.quantiles.sub(forecast_garbage_target.quantiles).abs().amax().item<double>();
  std::cout << "predict() ignores target, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
quantiles.sizes() = [48, 3, 3]
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
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(level + 0.5 * position);
    }
  }
  data::TimeSeriesBatch out{};
  out.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.encoder_target = pluss::make_series(encoder_flat, {n, encoder_length}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train_ts = make_synthetic(48, 6, 3, 5ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.0;
  options.cfg = f32();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  model.learn(models::TFTBatch(train_ts), train_options);

  const auto forecast_native = model.predict(models::TFTBatch(train_ts));
  // The data::Batch overload is exactly TFTBatch(batch) first, then the
  // native overload.
  const data::Batch core = dataset.data().batch();
  const auto forecast_core = model.predict(core);
  const double diff = forecast_native.quantiles.sub(forecast_core.quantiles).abs().amax().item<double>();
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
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

models::TFTBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                 std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(level + 0.5 * position);
    }
  }
  models::TFTBatch out{};
  out.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train = make_synthetic(48, 6, 3, 5ULL);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.dropout = 0.1; // nonzero dropout -- determinism still must hold at inference time.
  options.cfg = f32();
  options.seed = 5;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  const auto first = model.predict(train);
  const auto second = model.predict(train);
  const double diff = first.quantiles.sub(second.quantiles).abs().amax().item<double>();
  std::cout << "repeated predict() on a trained model is deterministic, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
repeated predict() on a trained model is deterministic, diff = 0
```

### Example 4

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

models::TFTBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                 std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(level + 0.5 * position);
    }
  }
  models::TFTBatch out{};
  out.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  // Trained with a 6-step lookback window.
  const auto train = make_synthetic(32, 6, 3, 11ULL);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.cfg = f32();

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  // Predicting with an 8-step lookback window instead -- not rejected; the
  // LSTM encoder and attention layer simply run over the longer sequence.
  const auto wrong_shape = make_synthetic(32, 8, 3, 12ULL);
  const auto forecast = model.predict(wrong_shape);
  std::cout << "quantiles.sizes() = [" << forecast.quantiles.sizes()[0] << ", " << forecast.quantiles.sizes()[1]
             << ", " << forecast.quantiles.sizes()[2] << "]\n";
  std::cout << "attention_weights.sizes() = [" << forecast.attention_weights.sizes()[0] << ", "
             << forecast.attention_weights.sizes()[1] << ", " << forecast.attention_weights.sizes()[2] << "]\n";
  const double attn_row_sum_error = forecast.attention_weights.sum(-1, false).sub(1.0).abs().amax().item<double>();
  std::cout << "attention_weights rows still sum to ~1, max error = " << attn_row_sum_error << "\n";

  return 0;
}
```

Output

```text
quantiles.sizes() = [32, 3, 3]
attention_weights.sizes() = [32, 3, 11]
attention_weights rows still sum to ~1, max error = 1.19209e-07
```

### Example 5

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <iostream>
#include <random>
#include <vector>

namespace {
namespace ice = pluss::ai::ice;
namespace models = ice::models;
using pluss::ScalarType;

pluss::Config f32() { return pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

models::TFTBatch make_synthetic(std::int64_t n, std::int64_t encoder_length, std::int64_t decoder_length,
                                 std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> level_dist(-1.0, 1.0);
  std::vector<float> static_flat(static_cast<std::size_t>(n));
  std::vector<float> encoder_flat(static_cast<std::size_t>(n * encoder_length));
  std::vector<float> decoder_flat(static_cast<std::size_t>(n * decoder_length));
  std::vector<float> target_flat(static_cast<std::size_t>(n * decoder_length));
  for (std::int64_t i = 0; i < n; ++i) {
    const double level = level_dist(rng);
    static_flat[static_cast<std::size_t>(i)] = static_cast<float>(level);
    for (std::int64_t t = 0; t < encoder_length; ++t) {
      encoder_flat[static_cast<std::size_t>(i * encoder_length + t)] = static_cast<float>(level);
    }
    for (std::int64_t t = 0; t < decoder_length; ++t) {
      const double position = static_cast<double>(t + 1) / static_cast<double>(decoder_length);
      decoder_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(position);
      target_flat[static_cast<std::size_t>(i * decoder_length + t)] = static_cast<float>(level + 0.5 * position);
    }
  }
  models::TFTBatch out{};
  out.static_inputs = pluss::make_series(static_flat, {n, 1}, f32());
  out.encoder_inputs = pluss::make_series(encoder_flat, {n, encoder_length, 1}, f32());
  out.decoder_inputs = pluss::make_series(decoder_flat, {n, decoder_length, 1}, f32());
  out.target = pluss::make_series(target_flat, {n, decoder_length}, f32());
  return out;
}
} // namespace

int main() {
  const auto train = make_synthetic(32, 6, 3, 11ULL);

  models::TFTOptions options{};
  options.static_size = 1;
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.lstm_layers = 1;
  options.attention_heads = 2;
  options.cfg = f32();

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  // encoder_inputs has 2 features per timestep, but the model was trained
  // with encoder_size = 1 -- rejected, instead of silently reading only
  // the first feature and ignoring the second.
  models::TFTBatch wrong_width = train;
  wrong_width.encoder_inputs =
      pluss::series::unite(std::vector<pluss::Series>{train.encoder_inputs, train.encoder_inputs}, -1);
  bool threw = false;
  try {
    const auto forecast = model.predict(wrong_width);
    static_cast<void>(forecast);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "predict() with encoder_inputs wider than encoder_size throws = " << threw << "\n";

  return 0;
}
```

Output

```text
predict() with encoder_inputs wider than encoder_size throws = 1
```

## See Also

- [`TFTForecast`](../TFTForecast.md) -- the returned type.
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
