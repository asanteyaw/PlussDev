<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c8d74d0f5e0e -->
# `pluss::ai::ice::Model::to`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
pluss::ai::ice::Model& pluss::ai::ice::Model::to(pluss::Platform platform);
pluss::ai::ice::Model& pluss::ai::ice::Model::to(pluss::ScalarType dtype);
pluss::ai::ice::Model& pluss::ai::ice::Model::to(pluss::Platform platform, pluss::ScalarType dtype);
```

## What It Does

Moves every registered parameter and module onto `platform`/`dtype` in
place. [`learn`](learn.md) calls this for you (see
[`ExecutionPolicy::platform`](../../options/ExecutionPolicy.md),
resolved via [`resolve_platform`](../../platform/resolve_platform.md));
exposed publicly as an escape hatch for callers who want to place a
model before/without going through `learn` -- for example, to warm it up
on a GPU ahead of a [`save`](save.md)/[`load`](load.md) round trip.
Returns `*this`, for chaining.

## Parameters

`Platform platform`
: The target platform.

`ScalarType dtype`
: The target dtype.

## Returns

`Model&` -- a reference to `*this`.

## Errors

Throws if the target platform/dtype combination isn't supported by this
model's parameters (for example, a platform with no accelerator
present).

## Practical Examples

Three examples, one per overload.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  model.to(pluss::pCPU);
  const pluss::Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});
  std::cout << "predictions:\n" << model.infer(one_row) << "\n";

  return 0;
}
```

Output

```text
predictions:
pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[3.0000358]]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  model.to(pluss::ScalarType::Float64);
  const pluss::Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});
  std::cout << "predictions:\n" << model.infer(one_row) << "\n";

  return 0;
}
```

Output

```text
predictions:
pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3.0000359]]
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  model.to(pluss::pCPU, pluss::ScalarType::Float64);
  const pluss::Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});
  std::cout << "predictions:\n" << model.infer(one_row) << "\n";

  return 0;
}
```

Output

```text
predictions:
pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3.0000034]]
```

## See Also

- [`resolve_platform`](../../platform/resolve_platform.md) -- what
  `learn` resolves `ExecutionPolicy::platform` through before calling
  this.
- [`ExecutionPolicy::platform`](../../options/ExecutionPolicy.md) -- the
  `TrainPlatform` request `learn` resolves.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="test_step.html">
    <span class="label">Previous</span>
    <span class="title">test_step</span>
  </a>
  <a class="next-link" href="train.html">
    <span class="label">Next</span>
    <span class="title">train</span>
  </a>
</div>
<!-- apiref:nav:end -->
