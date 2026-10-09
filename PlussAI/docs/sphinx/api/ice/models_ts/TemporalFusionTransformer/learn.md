<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aa6fd1226be0 -->
# `pluss::ai::ice::models::TemporalFusionTransformer::learn`

[Back to `pluss::ai::ice::models::TemporalFusionTransformer`](../TemporalFusionTransformer.md)

## Signature

```cpp
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const TFTBatch& train, const TFTTrainOptions& options = {});
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const TFTBatch& train, const TFTBatch& val, const TFTTrainOptions& options = {});
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const pluss::ai::ice::data::Batch& train, const TFTTrainOptions& options = {});
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const pluss::ai::ice::data::Batch& train, const pluss::ai::ice::data::Batch& val,
    const TFTTrainOptions& options = {});
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const pluss::ai::ice::data::TimeSeriesBatch& train, const TFTTrainOptions& options = {});
void pluss::ai::ice::models::TemporalFusionTransformer::learn(
    const pluss::ai::ice::data::TimeSeriesBatch& train, const pluss::ai::ice::data::TimeSeriesBatch& val,
    const TFTTrainOptions& options = {});
```

## What It Does

Trains the model in place on `train` against the pinball (quantile)
loss over [`TFTOptions::quantiles`](../TFTOptions.md), leaving it
[`ready()`](ready.md) afterward. The two [`TFTBatch`](../TFTBatch.md)-taking
overloads are the primitive every other overload is built from --
each `data::Batch`/`data::TimeSeriesBatch` overload simply constructs
a `TFTBatch` from its argument(s) and forwards, with no behavior
difference from doing that conversion yourself (see Example 3).
Passing `val` explicitly (either two-argument overload) evaluates on
that exact held-out batch every epoch for
[`loop_regularization`](../TFTTrainOptions.md)/early stopping,
bypassing [`ValidationPolicy::split_ratio`](../../options/ValidationPolicy.md)
entirely; the single-batch overloads instead carve a split out of
`train` itself per `options.validation.split_ratio`. Quantile
*spread* calibration tends to need a more generous epoch/patience
budget than median accuracy alone -- see
[`TFTTrainOptions`'s own Guidance](../TFTTrainOptions.md).

## Parameters

`const TFTBatch& train` / `const data::Batch& train` / `const data::TimeSeriesBatch& train`
: The training batch, in whichever of the three shapes is convenient
  -- see [`TFTBatch`](../TFTBatch.md)'s own converting constructors.

`const TFTBatch& val` / `const data::Batch& val` / `const data::TimeSeriesBatch& val`
: An explicit held-out validation batch, in the same shape as `train`.

`const TFTTrainOptions& options`
: Batch size, learning rate, optimizer, and the shared validation/
  regularization/checkpoint/execution policies -- see
  [`TFTTrainOptions`](../TFTTrainOptions.md) field by field.

## Returns

`void` -- mutates the model in place; [`ready()`](ready.md) becomes
`true` afterward.

## Errors

- Throws if `train.encoder_inputs`/`decoder_inputs`/`target` is
  undefined, or if `train.static_inputs` is undefined while
  [`TFTOptions::static_size`](../TFTOptions.md) `> 0`.
- Throws if `train`'s (or an explicitly-passed `val`'s)
  `encoder_inputs`/`decoder_inputs`/`static_inputs` feature *width*
  disagrees with
  [`TFTOptions::encoder_size`/`decoder_size`/`static_size`](../TFTOptions.md)
  -- see [`predict`'s own Example 5](predict.md#example-5) for the
  same check exercised directly.
- Throws if `options.execution.max_epochs`/`options.batch_size`/
  `options.validation.every_n_epochs` is not positive, or if the
  single-batch overload's `validation.split_ratio` leaves an empty
  train or validation split.
- Throws per [`ExecutionPolicy::fail_on_non_finite`](../../options/ExecutionPolicy.md)
  if a non-finite loss/gradient appears during training (on by
  default for this model).
- Unlike [`AutoregressiveForecaster::learn`](../AutoregressiveForecaster/learn.md),
  `train`/`val`'s lookback/horizon *lengths* are still not validated
  against [`TFTOptions`](../TFTOptions.md) -- see
  [`predict`'s own Errors](predict.md#errors) for why that one
  dimension stays deliberately unchecked.

## Practical Examples

Four examples: the single-batch overload taking a model from
un-trained to `ready()`, the two-batch overload training against an
explicit validation batch instead of an internal split, the
`data::TimeSeriesBatch` convenience overload producing byte-identical
predictions to converting to [`TFTBatch`](../TFTBatch.md) by hand
first, and a batch whose `decoder_inputs` is wider than `decoder_size`
being rejected before training starts.

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
  std::cout << "ready() before learn() = " << model.ready() << "\n";

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  std::cout << "ready() after learn() = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() before learn() = 0
ready() after learn() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <cmath>
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
  const auto val = make_synthetic(12, 6, 3, 9ULL);

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
  train_options.execution.max_epochs = 30;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 5;

  // Explicit held-out validation batch, evaluated every epoch for early
  // stopping, instead of carving a split out of `train`.
  model.learn(train, val, train_options);
  std::cout << "ready() = " << model.ready() << "\n";

  const double val_loss = model.assess(val);
  std::cout << "assess(val) is finite = " << std::isfinite(val_loss) << "\n";

  return 0;
}
```

Output

```text
ready() = 1
assess(val) is finite = 1
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

  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;

  // Route A: convert to TFTBatch explicitly, call the native overload.
  models::TemporalFusionTransformer model_a(options);
  model_a.learn(models::TFTBatch(train_ts), train_options);

  // Route B: the data::TimeSeriesBatch convenience overload -- it just does
  // the same conversion internally before forwarding.
  models::TemporalFusionTransformer model_b(options);
  model_b.learn(train_ts, train_options);

  const auto forecast_a = model_a.predict(train_ts);
  const auto forecast_b = model_b.predict(train_ts);
  const double diff = forecast_a.quantiles.sub(forecast_b.quantiles).abs().amax().item<double>();
  std::cout << "convenience overload matches native overload exactly, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
convenience overload matches native overload exactly, diff = 0
```

### Example 4

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

  // decoder_inputs has 2 features per timestep, but TFTOptions::decoder_size
  // above is 1 -- learn() rejects this before training even starts.
  models::TFTBatch wrong_width = train;
  wrong_width.decoder_inputs =
      pluss::series::unite(std::vector<pluss::Series>{train.decoder_inputs, train.decoder_inputs}, -1);

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;

  bool threw = false;
  try {
    model.learn(wrong_width, train_options);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "learn() with decoder_inputs wider than decoder_size throws = " << threw << "\n";

  return 0;
}
```

Output

```text
learn() with decoder_inputs wider than decoder_size throws = 1
```

## See Also

- [`TFTBatch`](../TFTBatch.md) -- the primitive batch type, and its
  `data::Batch`/`data::TimeSeriesBatch` converting constructors.
- [`TFTTrainOptions`](../TFTTrainOptions.md) -- every training knob.
- [`predict`](predict.md) / [`assess`](assess.md) -- the usual next
  calls on a trained model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
