<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 83fc0bd753a7 -->
# `pluss::ai::ice::models::AutoregressiveForecaster::save`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
void pluss::ai::ice::models::AutoregressiveForecaster::save(std::string_view filepath) const;
```

## What It Does

Writes two files: `filepath` itself (a small text header --
[`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)'s
fields, enough to reconstruct the model's architecture) and
`filepath + ".weights"` (the trained parameters). Both are required by
[`load`](load.md) to reconstruct an equivalent, `ready()` model.

## Parameters

`std::string_view filepath`
: Where to write the header; the weights file is derived from it by
  appending `".weights"`. Must be non-empty.

## Returns

`void`.

## Errors

- Throws if the model has not been trained (`ready() == false`) --
  there is nothing meaningful to save yet.
- Throws if `filepath` is empty, or if either file cannot be opened/
  written.

## Practical Examples

Three examples: `save` refusing to run before `learn`, a full save/
load round trip reproducing identical predictions, and `filepath`
still being validated even once the model itself is trained.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  models::AutoregressiveForecasterOptions options{};
  options.encoder_size = 1;
  options.decoder_size = 1;
  options.encoder_length = 6;
  options.decoder_length = 3;
  options.hidden_size = 8;
  options.rnn_layers = 1;
  options.cfg = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  models::AutoregressiveForecaster model(options);
  const auto path = std::filesystem::temp_directory_path() / "ice_ar_save_untrained.artifact";
  bool threw = false;
  try {
    model.save(path.string());
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "save() before learn() throws = " << threw << "\n";

  return 0;
}
```

Output

```text
save() before learn() throws = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <filesystem>
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
  options.hidden_size = 8;
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
  model.learn(train_ts, train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_ar_save_roundtrip.artifact";
  model.save(path.string());
  std::cout << "header file exists = " << std::filesystem::exists(path) << "\n";
  std::cout << "weights file exists = " << std::filesystem::exists(path.string() + ".weights") << "\n";

  const auto before = model.predict(train_ts);
  const auto loaded = models::AutoregressiveForecaster::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".weights");

  std::cout << "loaded.ready() = " << loaded.ready() << "\n";
  const auto after = loaded.predict(train_ts);
  const double diff = before.mean.sub(after.mean).abs().amax().item<double>();
  std::cout << "save/load round-trip reproduces identical predictions, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
header file exists = 1
weights file exists = 1
loaded.ready() = 1
save/load round-trip reproduces identical predictions, diff = 0
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
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

  bool threw = false;
  try {
    model.save("");
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "save(\"\") on a trained model still throws = " << threw << "\n";

  return 0;
}
```

Output

```text
save("") on a trained model still throws = 1
```

## See Also

- [`load`](load.md) -- the inverse.
- [`ready`](ready.md) -- what `save` requires to be `true` first.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ready.html">
    <span class="label">Previous</span>
    <span class="title">ready</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
<!-- apiref:nav:end -->
