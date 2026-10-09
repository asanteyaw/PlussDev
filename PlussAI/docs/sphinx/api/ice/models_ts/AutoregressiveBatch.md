# `pluss::ai::ice::models::AutoregressiveBatch`

[`AutoregressiveForecaster`](AutoregressiveForecaster.md)'s own batch
shape -- `static_inputs`/`encoder_inputs`/`decoder_inputs`/
`encoder_target`/`target`, constructible from a
[`data::Batch`](../data/Batch.md) or a
[`data::TimeSeriesBatch`](../data/TimeSeriesBatch.md), or built
directly from raw `Series`.

## Signature

```cpp
pluss::ai::ice::models::AutoregressiveBatch::AutoregressiveBatch() = default;
explicit pluss::ai::ice::models::AutoregressiveBatch::AutoregressiveBatch(const pluss::ai::ice::data::Batch& batch);
explicit pluss::ai::ice::models::AutoregressiveBatch::AutoregressiveBatch(const pluss::ai::ice::data::TimeSeriesBatch& batch);
```

## State

### `static_inputs`

`Series static_inputs = pluss::series::undefined()`
: `[N, static_size]`, or undefined when `static_size == 0`.

### `encoder_inputs`

`Series encoder_inputs = pluss::series::undefined()`
: `[N, encoder_length, encoder_size]`.

### `decoder_inputs`

`Series decoder_inputs = pluss::series::undefined()`
: `[N, decoder_length, decoder_size]`.

### `encoder_target`

`Series encoder_target = pluss::series::undefined()`
: `[N, encoder_length]` -- the target's own history over the lookback
  window, fed back into the RNN autoregressively during training.

### `target`

`Series target = pluss::series::undefined()`
: `[N, decoder_length]` -- required for
  [`learn`](AutoregressiveForecaster/learn.md)/
  [`assess`](AutoregressiveForecaster/assess.md), ignored by
  [`predict`](AutoregressiveForecaster/predict.md).

## Methods

None -- `AutoregressiveBatch` is a plain batch value; see `## State`
above. The two converting constructors read each field off the source
type by its matching
[`field_names`](../data/Batch.md)/`TimeSeriesBatch` member, leaving
anything absent from the source undefined on the result.

## Practical Examples

Three examples: building one directly from raw tensors, converting
from a [`data::TimeSeriesBatch`](../data/TimeSeriesBatch.md) that
never set `static_inputs`, and converting from a generic
[`data::Batch`](../data/Batch.md) with no `"static_inputs"` field at
all -- both conversions leave `static_inputs` undefined rather than
inventing a zero tensor.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  models::AutoregressiveBatch batch{};
  batch.static_inputs = pluss::series::randn({4, 1}, f32, 1ULL);
  batch.encoder_inputs = pluss::series::randn({4, 8, 1}, f32, 2ULL);
  batch.decoder_inputs = pluss::series::randn({4, 4, 1}, f32, 3ULL);
  batch.encoder_target = pluss::series::randn({4, 8}, f32, 4ULL);
  batch.target = pluss::series::randn({4, 4}, f32, 5ULL);

  std::cout << "static_inputs.defined() = " << batch.static_inputs.defined() << "\n";
  std::cout << "encoder_inputs.sizes()[1] = " << batch.encoder_inputs.sizes()[1] << "\n";
  std::cout << "decoder_inputs.sizes()[1] = " << batch.decoder_inputs.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
static_inputs.defined() = 1
encoder_inputs.sizes()[1] = 8
decoder_inputs.sizes()[1] = 4
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  data::TimeSeriesBatch ts_batch{};
  ts_batch.encoder_inputs = pluss::series::randn({4, 8, 1}, f32, 2ULL);
  ts_batch.decoder_inputs = pluss::series::randn({4, 4, 1}, f32, 3ULL);
  ts_batch.encoder_target = pluss::series::randn({4, 8}, f32, 4ULL);
  ts_batch.target = pluss::series::randn({4, 4}, f32, 5ULL);
  // static_inputs deliberately left undefined on the TimeSeriesBatch.

  const models::AutoregressiveBatch batch(ts_batch);
  std::cout << "static_inputs.defined() = " << batch.static_inputs.defined() << "\n";
  const double diff = batch.encoder_inputs.sub(ts_batch.encoder_inputs).abs().amax().item<double>();
  std::cout << "encoder_inputs matches source TimeSeriesBatch, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
static_inputs.defined() = 0
encoder_inputs matches source TimeSeriesBatch, diff = 0
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  data::Batch core(data::Layout::Panel);
  core.set(std::string(data::field_names::encoder_inputs), pluss::series::randn({4, 8, 1}, f32, 2ULL));
  core.set(std::string(data::field_names::decoder_inputs), pluss::series::randn({4, 4, 1}, f32, 3ULL));
  core.set(std::string(data::field_names::encoder_target), pluss::series::randn({4, 8}, f32, 4ULL));
  core.set_target(pluss::series::randn({4, 4}, f32, 5ULL));
  // No "static_inputs" field set on the core data::Batch at all.

  const models::AutoregressiveBatch batch(core);
  std::cout << "static_inputs.defined() = " << batch.static_inputs.defined() << "\n";
  std::cout << "target.sizes()[0] = " << batch.target.sizes()[0] << "\n";

  return 0;
}
```

Output

```text
static_inputs.defined() = 0
target.sizes()[0] = 4
```

## Changelog

- **0.1** -- `AutoregressiveBatch`: `static_inputs`/`encoder_inputs`/
  `decoder_inputs`/`encoder_target`/`target`, plus the `data::Batch`/
  `data::TimeSeriesBatch` converting constructors.

## See Also

- [`AutoregressiveForecaster::learn`/`predict`/`assess`](AutoregressiveForecaster.md)
  -- every consumer.
- [`data::Batch`](../data/Batch.md) / [`data::TimeSeriesBatch`](../data/TimeSeriesBatch.md)
  -- the two source types the converting constructors read.
- [`TFTBatch`](TFTBatch.md) -- the analogous batch shape for
  `TemporalFusionTransformer`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
