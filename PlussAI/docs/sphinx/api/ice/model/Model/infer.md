<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d6668e8a8a2 -->
# `pluss::ai::ice::Model::infer`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
pluss::Series pluss::ai::ice::Model::infer(pluss::ai::ice::Source& source) const;
pluss::Series pluss::ai::ice::Model::infer(const pluss::Series& x, std::int64_t chunk_size = 0) const;
pluss::Series pluss::ai::ice::Model::infer(const pluss::table::DataFrame& df, std::string_view target = {}, std::int64_t chunk_size = 0) const;
```

## What It Does

Runs inference in eval mode -- every overload is a thin wrapper around
`Coach(...).infer(*this, ...)`.

- The `Series` overload's `chunk_size > 0` bounds memory for large `x`
  by looping over row-chunks (each chunk is still one batched forward
  call, not a per-row loop) and concatenating the results; `0` (the
  default) runs the whole input as one batch.
- The `DataFrame` overload mirrors `learn(df, target, options)`: pulls
  every column except `target` as a feature, applies whatever
  standardization `learn` fit (a no-op if the model wasn't trained with
  `standardize = true`), then delegates to the `Series` overload.
  `target` defaults to empty -- genuinely new data to predict on
  typically has no target column at all; pass the same name used in
  `learn()` when reusing that same `DataFrame` (right after
  [`assess`](assess.md), for example), so its label column isn't
  mistaken for a feature.
- The `Source` overload infers over an already-constructed
  [`Source`](../../source/Source.md)'s own `infer_batches()`.

## Parameters

`Source& source`
: An already-constructed data source.

`const Series& x`
: The input to run inference on.

`std::int64_t chunk_size = 0`
: Row-chunk size for the `Series`/`DataFrame` overloads; `0` runs the
  whole input in one batch.

`const pluss::table::DataFrame& df`
: A `DataFrame` whose columns are features (plus, optionally, a target
  column to exclude via `target`).

`std::string_view target = {}`
: The column to exclude as a feature, if any.

## Returns

`Series` -- the model's predictions.

## Errors

Throws if the model has never been trained/loaded (an undefined
parameter), or on a malformed input shape.

## Practical Examples

Four examples, one per overload plus `chunk_size`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline trained_model() {
  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);
  return model;
}

int main() {
  ice::Outline model = trained_model();
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});

  const auto predictions = model.infer(x);
  std::cout << "shape = [" << predictions.sizes()[0] << "," << predictions.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
shape = [4,1]
```

### Example 2

The same call with `chunk_size = 2` -- same result shape, computed two
rows at a time instead of all four at once.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline trained_model() {
  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);
  return model;
}

int main() {
  ice::Outline model = trained_model();
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});

  const auto predictions = model.infer(x, 2);
  std::cout << "shape = [" << predictions.sizes()[0] << "," << predictions.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
shape = [4,1]
```

### Example 3

The `DataFrame` overload, `target` left at its default (genuinely new
data, no label column).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline trained_model() {
  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);
  return model;
}

int main() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  ice::Outline model = trained_model();
  const DataFrame df({
      {"a", Slate(pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}))},
      {"b", Slate(pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}))},
  });

  const auto predictions = model.infer(df);
  std::cout << "shape = [" << predictions.sizes()[0] << "," << predictions.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
shape = [4,1]
```

### Example 4

The `Source` overload, over a [`TensorSource`](../../source/TensorSource.md)
constructed for inference.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline trained_model() {
  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);
  return model;
}

int main() {
  ice::Outline model = trained_model();
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});

  ice::BatchPlan plan{};
  plan.batch_size = 4;
  ice::TensorSource source(x, plan);

  const auto predictions = model.infer(source);
  std::cout << "shape = [" << predictions.sizes()[0] << "," << predictions.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
shape = [4,1]
```

## See Also

- [`learn`](learn.md) -- trains the model this infers from.
- [`assess`](assess.md) -- `infer` plus a loss/metric against known
  targets.
- [`TrainOptions::standardize`](../../options/TrainOptions.md) -- what
  the `DataFrame` overload reapplies automatically.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
  <a class="next-link" href="infer_step.html">
    <span class="label">Next</span>
    <span class="title">infer_step</span>
  </a>
</div>
<!-- apiref:nav:end -->
