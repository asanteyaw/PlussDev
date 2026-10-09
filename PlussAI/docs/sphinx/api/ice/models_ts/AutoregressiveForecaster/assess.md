<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 619ed1d22ba8 -->
# `pluss::ai::ice::models::AutoregressiveForecaster::assess`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
[[nodiscard]] double pluss::ai::ice::models::AutoregressiveForecaster::assess(
    const AutoregressiveBatch& batch) const;
[[nodiscard]] double pluss::ai::ice::models::AutoregressiveForecaster::assess(
    const pluss::ai::ice::data::Batch& batch) const;
[[nodiscard]] double pluss::ai::ice::models::AutoregressiveForecaster::assess(
    const pluss::ai::ice::data::TimeSeriesBatch& batch) const;
```

## What It Does

Returns the Gaussian negative log-likelihood of `batch.target` under
the model's own predicted `mean`/`scale` for `batch` -- the same
objective [`learn`](learn.md) minimizes, but computed once rather than
backpropagated through. Unlike [`predict`](predict.md), `assess`
*does* read `batch.target`; passing a batch with the right inputs but
an unrelated target scores measurably worse (see Example 3). The two
`data::Batch`/`data::TimeSeriesBatch` overloads are convenience
wrappers that construct an [`AutoregressiveBatch`](../AutoregressiveBatch.md)
and forward, with no behavior difference (see Example 2).

## Parameters

`const AutoregressiveBatch& batch` / `const data::Batch& batch` / `const data::TimeSeriesBatch& batch`
: The batch to score, including its `target` -- in whichever of the
  three shapes is convenient.

## Returns

`double` -- the mean Gaussian NLL over `batch`, finite for any
well-formed batch.

## Errors

- Throws if `batch`'s tensor shapes disagree with
  [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md),
  or if `batch.target` is undefined.

## Practical Examples

Three examples: a finite loss on the data the model was trained on,
the `data::TimeSeriesBatch` overload matching the native overload
exactly, and the loss rising on a batch whose target was swapped for
unrelated noise.

### Example 1

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
  train_options.execution.max_epochs = 30;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(train_ts, train_options);

  const double loss = model.assess(models::AutoregressiveBatch(train_ts));
  std::cout << "assess(train) is finite = " << std::isfinite(loss) << "\n";

  return 0;
}
```

Output

```text
assess(train) is finite = 1
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
  train_options.execution.max_epochs = 30;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(train_ts, train_options);

  const double loss_native = model.assess(models::AutoregressiveBatch(train_ts));
  const double loss_ts = model.assess(train_ts);
  std::cout << "data::TimeSeriesBatch overload matches native overload exactly: "
            << (loss_native == loss_ts) << "\n";

  return 0;
}
```

Output

```text
data::TimeSeriesBatch overload matches native overload exactly: 1
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
  options.dropout = 0.0;
  options.seed = 7;
  options.cfg = f32();

  models::AutoregressiveForecaster model(options);
  models::AutoregressiveTrainOptions train_options{};
  train_options.execution.max_epochs = 30;
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 3;
  model.learn(train_ts, train_options);

  const double loss_on_fit_data = model.assess(train_ts);

  // A batch with the SAME inputs, but a target replaced with unrelated
  // noise -- a fit model should score that much worse.
  models::AutoregressiveBatch mismatched(train_ts);
  mismatched.target = pluss::series::randn(mismatched.target.sizes(), f32(), 123ULL).mul(5.0);
  const double loss_on_mismatched = model.assess(mismatched);

  std::cout << "loss on fit data < loss on mismatched target: "
            << (loss_on_fit_data < loss_on_mismatched) << "\n";

  return 0;
}
```

Output

```text
loss on fit data < loss on mismatched target: 1
```

## See Also

- [`learn`](learn.md) -- minimizes this same objective.
- [`predict`](predict.md) -- the forecast itself, ignoring `target`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
