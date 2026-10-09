<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9df1b0ee306a -->
# `pluss::ai::ice::Outline::forward`

[Back to `pluss::ai::ice::Outline`](../Outline.md)

## Signature

```cpp
pluss::Series pluss::ai::ice::Outline::forward(const pluss::Series& x) const override;
using pluss::ai::ice::Model::forward;
```

## What It Does

Overrides [`Model::forward`](../../model/Model/forward.md)'s `Series`
overload by delegating to the composed
[`Schedule`](../../../nn/module/Schedule.md)'s own `forward` -- running
`x` through every stage in order and returning the last stage's output.
`Outline` does not reimplement any layer logic; it is a thin adapter
that lets a plain chain of existing layers satisfy `Model`'s interface.

The `using Model::forward;` declaration alongside it brings back the
inherited `Batch` overload, which this override would otherwise hide at
`Outline`'s own scope -- see
[`Outline`'s own Introduction](Introduction.md#why-the-using-declaration)
for why that is necessary in any `Model` subclass that overrides only
one overload of `forward`.

## Parameters

`const Series& x`
: The input tensor, passed straight to the composed `Schedule`'s own
  `forward`. Must satisfy whatever input type/shape the first stage in
  the schedule expects -- `Schedule::run` throws its own error
  otherwise, not a message specific to `Outline`.

## Returns

`Series` -- the last stage's output, unchanged.

## Errors

- Whatever the composed `Schedule` throws for a mismatched input type
  or shape at any stage (see
  [`Schedule::run`](../../../nn/module/Schedule/run.md)) -- `Outline`
  adds no validation of its own on top.

## Practical Examples

Three examples: a direct call, confirming the inherited `Batch` overload
still reaches this same implementation through a `Model&`, and an
end-to-end run where `forward` and [`infer`](../../model/Model/infer.md)
agree on a trained model.

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

The inherited `Batch` overload (brought back by `using Model::forward;`)
reaches this exact `Series` implementation when called through a
`Model&` -- `Model::forward(const Batch&)`'s default body is
`forward(batch.inputs())`, so both paths produce an identical result for
the same data.

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
  batch.set_targets(pluss::make_series({3.0f}, {1, 1}));

  // Reached through Model&, via the using-declaration Outline brings in --
  // see Outline's own Introduction for why that is necessary.
  ice::Model& base = model;
  const pluss::Series direct = model.forward(batch.inputs());
  const pluss::Series via_model = base.forward(batch);
  const double diff = direct.sub(via_model).abs().amax().item<double>();
  std::cout << "diff = " << diff << "\n";

  return 0;
}
```

Output

```text
diff = 0
```

### Example 3

After [`learn`](../../model/Model/learn.md) has trained the model,
`forward` and [`infer`](../../model/Model/infer.md) produce the same
value -- `infer` is `forward` plus input-shape normalization, nothing
more, for a model whose `forward` already takes a plain `Series`.

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

  const pluss::Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});
  std::cout << "forward(one_row):\n" << model.forward(one_row) << "\n";
  std::cout << "infer(one_row):\n" << model.infer(one_row) << "\n";

  return 0;
}
```

Output

```text
forward(one_row):
pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[3.0000358]]
infer(one_row):
pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[3.0000358]]
```

## See Also

- [`Model::forward`](../../model/Model/forward.md) -- the base class
  method this overrides, including the general name-hiding gotcha.
- [`schedule`](schedule.md) -- the composed object this delegates to.
- [`Schedule::run`](../../../nn/module/Schedule/run.md) -- what actually
  walks the stages.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="schedule.html">
    <span class="label">Next</span>
    <span class="title">schedule</span>
  </a>
</div>
<!-- apiref:nav:end -->
