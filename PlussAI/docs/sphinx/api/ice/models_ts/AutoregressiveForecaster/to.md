<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23fefc566cf5 -->
# `pluss::ai::ice::models::AutoregressiveForecaster::to`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
pluss::ai::ice::models::AutoregressiveForecaster& pluss::ai::ice::models::AutoregressiveForecaster::to(
    pluss::ai::ice::Platform platform);
```

## What It Does

Moves every parameter to `platform` in place, mutating this model and
returning `*this` so the call chains directly into the next one (see
Example 2). Calling it with the platform the model is already on is a
well-defined no-op -- predictions afterward are byte-identical (see
Example 1). [`ready()`](ready.md) is unaffected either way.

## Parameters

`Platform platform`
: The target platform (`pluss::pCPU`, or a GPU platform if this build
  and machine support one).

## Returns

`AutoregressiveForecaster&` -- `*this`, for chaining.

## Errors

None directly -- whatever moving a parameter's underlying `Series` to
`platform` would throw (for example, requesting an unsupported
platform on this build).

## Practical Examples

Three examples: `to(pCPU)` on an already-CPU model leaving predictions
unchanged, the returned reference chaining straight into `predict`,
and `ready()` surviving the call.

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
  const auto train_ts = make_synthetic(32, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 8;
  options.rnn_layers = 1;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train_ts, train_options);

  const auto before = model.predict(train_ts);
  model.to(pluss::pCPU);
  const auto after = model.predict(train_ts);
  const double diff = before.mean.sub(after.mean).abs().amax().item<double>();
  std::cout << "to(pCPU) on an already-CPU model is a no-op, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
to(pCPU) on an already-CPU model is a no-op, diff = 0
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
  const auto train_ts = make_synthetic(32, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 8;
  options.rnn_layers = 1;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train_ts, train_options);

  // to() returns *this by reference, so it chains directly into the next call.
  const auto forecast = model.to(pluss::pCPU).predict(train_ts);
  std::cout << "chained to().predict() shape = [" << forecast.mean.sizes()[0] << ", "
            << forecast.mean.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
chained to().predict() shape = [32, 3]
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
  const auto train_ts = make_synthetic(32, 6, 3, 11ULL);
  const auto dataset = data::TimeSeriesDataSet::from_series(train_ts);
  auto options = models::AutoregressiveForecasterOptions::from_dataset(dataset);
  options.hidden_size = 8;
  options.rnn_layers = 1;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train_ts, train_options);

  model.to(pluss::pCPU);
  std::cout << "ready() survives to(), still = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() survives to(), still = 1
```

## See Also

- [`ready`](ready.md) -- unaffected by `to`.
- [`predict`](predict.md) -- the usual call chained right after `to`
  when moving a loaded model to a different platform for inference.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
