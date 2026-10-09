<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e2fdd8c2f9e1 -->
# `pluss::ai::ice::Model::forward`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::Series pluss::ai::ice::Model::forward(const pluss::Series& x) const = 0;
virtual pluss::Series pluss::ai::ice::Model::forward(const pluss::ai::ice::Batch& batch) const;
```

## What It Does

The `Series` overload is pure virtual -- every `Model` subclass must
implement it; it is the one method every subclass genuinely has to
write. The `Batch` overload has a default implementation
(`forward(batch.inputs())`), virtual so it can still be overridden
directly for a model whose real input isn't a single tensor, but every
built-in call path (the four step methods) goes through it rather than
the `Series` overload directly.

## Parameters

`const Series& x`
: The input tensor, for the `Series` overload.

`const Batch& batch`
: The input batch, for the `Batch` overload -- reads `batch.inputs()`
  by default.

## Returns

`Series` -- the model's output.

## Errors

- The `Batch` overload's default throws if `batch` has no `"inputs"`
  field set (see [`Batch::inputs`](../../step/Batch/inputs.md)).

## Practical Examples

Two examples, one per overload, both via [`Outline`](../../outline/Outline.md).

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
  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});

  std::cout << "forward(x):\n" << model.forward(x) << "\n";

  return 0;
}
```

Output

```text
forward(x):
pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[-1.1561395]]
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
  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {1, 2}));

  std::cout << "forward(batch):\n" << model.forward(batch) << "\n";

  return 0;
}
```

Output

```text
forward(batch):
pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[0.90112948]]
```

### Example 3

A custom `Model` subclass only ever has to implement the `Series`
overload -- the `Batch` overload's default (`forward(batch.inputs())`)
is almost always enough. But overriding one overload of `forward` hides
the *other* one at the subclass's own scope in plain C++ (name hiding,
not a virtual-dispatch rule) -- without `using Model::forward;`, calling
`forward(batch)` directly on a concrete subclass instance fails to
compile, even though the inherited `Batch` overload is still there and
still virtual. Add the `using`-declaration shown below in any subclass
that might be called this way directly (every built-in call path, the
four step methods, is unaffected either way -- they call through a
`Model&`, where both overloads are always visible regardless).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

class DoubleHead final : public ice::Model {
 public:
  DoubleHead() : head_(2, 1) { register_module(*head_.get()); }

  using ice::Model::forward;
  pluss::Series forward(const pluss::Series& x) const override {
    return head_->forward(x).mul(2.0F);
  }

 private:
  nn::Fusion head_;
};

int main() {
  DoubleHead model;
  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});

  ice::Batch batch{};
  batch.set_inputs(x);

  const double diff = model.forward(x).sub(model.forward(batch)).abs().amax().item<double>();
  std::cout << "forward(x) == forward(batch)? diff = " << diff << "\n";

  return 0;
}
```

Output

```text
forward(x) == forward(batch)? diff = 0
```

## See Also

- [`Batch::inputs`](../../step/Batch/inputs.md) -- what the `Batch`
  overload's default reads.
- [`compute_loss`](compute_loss.md) -- typically called on `forward`'s
  result.
- [`Outline`](../../outline/Outline.md) -- the common-case subclass that
  only overrides the `Series` overload.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eval.html">
    <span class="label">Previous</span>
    <span class="title">eval</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
