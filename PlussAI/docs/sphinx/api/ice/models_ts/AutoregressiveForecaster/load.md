<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a28e9158ff2d -->
# `pluss::ai::ice::models::AutoregressiveForecaster::load`

[Back to `pluss::ai::ice::models::AutoregressiveForecaster`](../AutoregressiveForecaster.md)

## Signature

```cpp
[[nodiscard]] static pluss::ai::ice::models::AutoregressiveForecaster pluss::ai::ice::models::AutoregressiveForecaster::load(
    std::string_view filepath);
```

## What It Does

The inverse of [`save`](save.md): reads `filepath`'s header to
reconstruct an [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)
(no caller-supplied options needed at all), constructs a fresh model
from it, then loads `filepath + ".weights"` into its parameters. The
returned model is already [`ready()`](ready.md) -- no call to
[`learn`](learn.md) is needed or expected.

## Parameters

`std::string_view filepath`
: The header path originally passed to [`save`](save.md). Must be
  non-empty and refer to a file written by `save`; the weights file
  is derived from it by appending `".weights"`.

## Returns

[`AutoregressiveForecaster`](../AutoregressiveForecaster.md), by
value -- a fully independent model, `ready()` and in evaluation mode.

## Errors

- Throws if `filepath` is empty, cannot be opened, or does not start
  with this model's own artifact header (for example, a path that
  does not exist, or one written by a different model type).
- Throws if the weights file is missing or does not match the
  parameter shapes the header describes.

## Practical Examples

Three examples: an empty path being rejected immediately, a full
save/load round trip reproducing identical predictions, and the
loaded model enforcing its own reconstructed `encoder_length` even
though the caller never passed any options to `load` at all.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  bool threw = false;
  try {
    auto model = models::AutoregressiveForecaster::load("");
    static_cast<void>(model);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "load(\"\") throws = " << threw << "\n";

  return 0;
}
```

Output

```text
load("") throws = 1
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
  // encoder_length=6, decoder_length=3 -- not repeated anywhere near load().
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

  const auto path = std::filesystem::temp_directory_path() / "ice_ar_load_shapes.artifact";
  model.save(path.string());

  // Loaded with no access to `options` at all -- its own
  // encoder_length/decoder_length were reconstructed purely from the saved
  // header. A batch with the WRONG encoder_length (8 instead of 6) must
  // still be rejected, proving the loaded model enforces its own shape,
  // not whatever shape happens to be handed to it.
  const auto loaded = models::AutoregressiveForecaster::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".weights");

  const auto wrong_shape_ts = make_synthetic(32, 8, 3, 12ULL);
  bool threw = false;
  try {
    const auto forecast = loaded.predict(wrong_shape_ts);
    static_cast<void>(forecast);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "loaded model rejects the wrong encoder_length = " << threw << "\n";

  return 0;
}
```

Output

```text
loaded model rejects the wrong encoder_length = 1
```

## See Also

- [`save`](save.md) -- the inverse, and what header/weights pair this
  reads.
- [`AutoregressiveForecasterOptions`](../AutoregressiveForecasterOptions.md)
  -- every field reconstructed from the header.
- [`ready`](ready.md) -- always `true` on the returned model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="predict.html">
    <span class="label">Next</span>
    <span class="title">predict</span>
  </a>
</div>
<!-- apiref:nav:end -->
