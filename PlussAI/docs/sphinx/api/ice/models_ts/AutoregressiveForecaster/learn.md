<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c1552c42b911 -->
# `pluss::ai::ice::models::AutoregressiveForecaster::learn`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const AutoregressiveBatch& train, const AutoregressiveTrainOptions& options = {});
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const AutoregressiveBatch& train, const AutoregressiveBatch& val,
    const AutoregressiveTrainOptions& options = {});
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const pluss::ai::ice::data::Batch& train, const AutoregressiveTrainOptions& options = {});
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const pluss::ai::ice::data::Batch& train, const pluss::ai::ice::data::Batch& val,
    const AutoregressiveTrainOptions& options = {});
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const pluss::ai::ice::data::TimeSeriesBatch& train, const AutoregressiveTrainOptions& options = {});
void pluss::ai::ice::models::AutoregressiveForecaster::learn(
    const pluss::ai::ice::data::TimeSeriesBatch& train, const pluss::ai::ice::data::TimeSeriesBatch& val,
    const AutoregressiveTrainOptions& options = {});
```

## What It Does

Trains the model in place on `train`, leaving it
[`ready()`](ready.md) afterward. The two
[`AutoregressiveBatch`](../AutoregressiveBatch.md)-taking overloads are
the primitive every other overload is built from -- each
`data::Batch`/`data::TimeSeriesBatch` overload simply constructs an
`AutoregressiveBatch` from its argument(s) and forwards, with no
behavior difference from doing that conversion yourself (see Example
3). Passing `val` explicitly (either two-argument overload) evaluates
on that exact held-out batch every epoch for
[`loop_regularization`](../AutoregressiveTrainOptions.md)/early
stopping, bypassing
[`ValidationPolicy::split_ratio`](../../options/ValidationPolicy.md)
entirely; the single-batch overloads instead carve a split out of
`train` itself per `options.validation.split_ratio` (default `0.1`).

## Parameters

`const AutoregressiveBatch& train` / `const data::Batch& train` / `const data::TimeSeriesBatch& train`
: The training batch, in whichever of the three shapes is convenient
  -- see [`AutoregressiveBatch`](../AutoregressiveBatch.md)'s own
  converting constructors.

`const AutoregressiveBatch& val` / `const data::Batch& val` / `const data::TimeSeriesBatch& val`
: An explicit held-out validation batch, in the same shape as `train`.

`const AutoregressiveTrainOptions& options`
: Batch size, learning rate, optimizer, and the shared validation/
  regularization/checkpoint/execution policies -- see
  [`AutoregressiveTrainOptions`](../AutoregressiveTrainOptions.md)
  field by field.

## Returns

`void` -- mutates the model in place; [`ready()`](ready.md) becomes
`true` afterward.

## Errors

- Throws if `train`'s tensor shapes disagree with
  [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)'s
  own `static_size`/`encoder_size`/`decoder_size`/`encoder_length`/
  `decoder_length`.
- Throws per [`ExecutionPolicy::fail_on_non_finite`](../../options/ExecutionPolicy.md)
  if a non-finite loss/gradient appears during training (on by
  default for this model).

## Practical Examples

Three examples: the single-batch overload taking a model from
un-trained to `ready()`, the two-batch overload training against an
explicit validation batch instead of an internal split, and the
`data::TimeSeriesBatch` convenience overload producing byte-identical
predictions to converting to
[`AutoregressiveBatch`](../AutoregressiveBatch.md) by hand first.

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
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;
  const auto train_ts = make_synthetic(64, kEncoderLength, kDecoderLength, 11ULL);

  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  std::cout << "ready() before learn() = " << model.ready() << "\n";

  const models::AutoregressiveBatch train(train_ts);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
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
  constexpr std::int64_t kEncoderLength = 6;
  constexpr std::int64_t kDecoderLength = 3;
  const auto train_ts = make_synthetic(64, kEncoderLength, kDecoderLength, 11ULL);
  const auto val_ts = make_synthetic(16, kEncoderLength, kDecoderLength, 22ULL);

  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);

  const models::AutoregressiveBatch train(train_ts);
  const models::AutoregressiveBatch val(val_ts);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 40;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 5;
  train_options.seed = 3;

  // Explicit held-out validation batch -- this overload evaluates on `val`
  // every epoch for early stopping, instead of carving a split out of
  // `train` the way validation.split_ratio would.
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

models::AutoregressiveForecasterOptions make_options(const data::TimeSeriesDataSet& dataset) {
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 12;
  options.rnn_layers = 1;
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();
  return options;
}

models::AutoregressiveTrainOptions make_train_options() {
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  return train_options;
}
} // namespace

int main() {
  const auto train_ts = make_synthetic(64, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);

  // Route A: convert to AutoregressiveBatch explicitly, call the native overload.
  models::AutoregressiveForecaster model_a(make_options(dataset));
  model_a.learn(models::AutoregressiveBatch(train_ts), make_train_options());

  // Route B: the data::TimeSeriesBatch convenience overload -- it just does
  // the same conversion internally before forwarding.
  models::AutoregressiveForecaster model_b(make_options(dataset));
  model_b.learn(train_ts, make_train_options());

  const auto forecast_a = model_a.predict(train_ts);
  const auto forecast_b = model_b.predict(train_ts);
  const double mean_diff = forecast_a.mean.sub(forecast_b.mean).abs().amax().item<double>();
  std::cout << "convenience overload matches native overload exactly, diff = " << mean_diff << "\n";

  return 0;
}
```

Output

```text
convenience overload matches native overload exactly, diff = 0
```

## See Also

- [`AutoregressiveBatch`](../AutoregressiveBatch.md) -- the primitive
  batch type, and its `data::Batch`/`data::TimeSeriesBatch` converting
  constructors.
- [`AutoregressiveTrainOptions`](../AutoregressiveTrainOptions.md) --
  every training knob.
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
