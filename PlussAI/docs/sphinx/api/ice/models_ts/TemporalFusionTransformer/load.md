<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ee2022a4bea7 -->
# `pluss::ai::ice::models::TemporalFusionTransformer::load`

[Back to `pluss::ai::ice::models::TemporalFusionTransformer`](../TemporalFusionTransformer.md)

## Signature

```cpp
[[nodiscard]] static pluss::ai::ice::models::TemporalFusionTransformer pluss::ai::ice::models::TemporalFusionTransformer::load(
    std::string_view filepath);
```

## What It Does

The inverse of [`save`](save.md): reads `filepath`'s header to
reconstruct a [`TFTOptions`](../TFTOptions.md) (including the exact
`quantiles` list, in order -- see Example 3) with no caller-supplied
options needed at all, constructs a fresh model from it, then loads
`filepath + ".weights"` into its parameters. The returned model is
already [`ready()`](ready.md) -- no call to [`learn`](learn.md) is
needed or expected.

## Parameters

`std::string_view filepath`
: The header path originally passed to [`save`](save.md). Must be
  non-empty and refer to a file written by `save`; the weights file
  is derived from it by appending `".weights"`.

## Returns

[`TemporalFusionTransformer`](../TemporalFusionTransformer.md), by
value -- a fully independent model, `ready()` and in evaluation mode.

## Errors

- Throws if `filepath` is empty, cannot be opened, or does not start
  with this model's own artifact header (for example, a path that
  does not exist, or one written by a different model type).
- Throws if the weights file is missing or does not match the
  parameter shapes the header describes.

## Practical Examples

Three examples: an empty path being rejected immediately, a full
save/load round trip reproducing identical predictions, and a
non-default `quantiles` list round-tripping exactly even though the
caller never passes `TFTOptions` to `load` at all.

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
    auto model = models::TemporalFusionTransformer::load("");
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
  options.seed = 7;

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 15;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_tft_save_roundtrip.artifact";
  model.save(path.string());
  std::cout << "header file exists = " << std::filesystem::exists(path) << "\n";
  std::cout << "weights file exists = " << std::filesystem::exists(path.string() + ".weights") << "\n";

  const auto before = model.predict(train);
  const auto loaded = models::TemporalFusionTransformer::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".weights");

  std::cout << "loaded.ready() = " << loaded.ready() << "\n";
  const auto after = loaded.predict(train);
  const double diff = before.quantiles.sub(after.quantiles).abs().amax().item<double>();
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

#include <filesystem>
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
  options.quantiles = {0.05, 0.25, 0.5, 0.75, 0.95}; // 5 quantiles, not the default 3.
  options.cfg = f32();

  models::TemporalFusionTransformer model(options);
  models::TFTTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  model.learn(train, train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_tft_load_quantiles.artifact";
  model.save(path.string());

  // Loaded with no access to `options` at all -- its own quantile count is
  // reconstructed purely from the saved header.
  const auto loaded = models::TemporalFusionTransformer::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".weights");

  const auto forecast = loaded.predict(train);
  std::cout << "loaded model's quantiles last axis = " << forecast.quantiles.sizes()[2] << "\n";

  return 0;
}
```

Output

```text
loaded model's quantiles last axis = 5
```

## See Also

- [`save`](save.md) -- the inverse, and what header/weights pair this
  reads.
- [`TFTOptions`](../TFTOptions.md) -- every field reconstructed from
  the header, including `quantiles`.
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
