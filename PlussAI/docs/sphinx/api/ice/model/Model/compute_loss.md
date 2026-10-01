# `pluss::ai::ice::Model::compute_loss`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::Series pluss::ai::ice::Model::compute_loss(const pluss::Series& predictions, const pluss::Series& y, pluss::ai::ice::LossKind kind) const;
virtual pluss::Series pluss::ai::ice::Model::compute_loss(const pluss::Series& predictions, const pluss::Series& y, pluss::ai::ice::LossKind kind, double label_smoothing) const;
virtual pluss::Series pluss::ai::ice::Model::compute_loss(const pluss::Series& predictions, const pluss::ai::ice::Batch& batch, const pluss::ai::ice::StepContext& ctx) const;
```

## What It Does

The 3-argument overload is the real extension point: its default
dispatches on `kind` to the matching `nn::losses::` free function
(`MSE`, `BCEWithLogits`, `CrossEntropy`); override it for a custom
objective.

The 4-argument overload exists specifically for `label_smoothing`: when
`kind == LossKind::CrossEntropy` and `label_smoothing != 0.0`, its
default calls `nn::losses::CrossEntropy` with that smoothing applied
directly, *without* going through the (possibly overridden) 3-argument
overload -- a subclass that overrides only the 3-argument overload is
bypassed whenever smoothing is requested. For every other case (any
other `kind`, or `label_smoothing == 0.0`), it delegates to the
3-argument overload as a plain virtual call, so an override there is
respected as expected.

The `Batch`/`StepContext` overload is what every step method actually
calls: it requires `batch.has_targets()` and `ctx.loss_kind` to be set,
reads `label_smoothing` from `ctx.recipe->objective_regularization`
(when `ctx.recipe` is non-null) and forwards to the 4-argument overload,
then applies `l1_penalty`/`l2_penalty` (also from
`ctx.recipe->objective_regularization`) to the result.

## Parameters

`const Series& predictions`
: The model's output, typically `forward`'s result.

`const Series& y`
: The target to compare against.

`LossKind kind`
: Which loss to compute.

`double label_smoothing`
: Only meaningful for `kind == LossKind::CrossEntropy`; see
  [`ObjectiveRegularization::label_smoothing`](../../options/ObjectiveRegularization.md).

`const Batch& batch`
: Supplies both `predictions`' comparison target (`batch.targets()`) and
  (indirectly, through `ctx`) everything the other two parameters above
  stand for.

`const StepContext& ctx`
: Supplies `loss_kind` (required) and, when `ctx.recipe` is set,
  `label_smoothing`/`l1_penalty`/`l2_penalty`.

## Returns

`Series` -- the computed loss (a scalar, typically), already including
any L1/L2 penalty the `Batch`/`StepContext` overload applied.

## Errors

- The `Batch`/`StepContext` overload throws if `batch.has_targets()` is
  `false`, or if `ctx.loss_kind` is unset.

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

  ice::Outline model(nn::Fusion(2, 3));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, {3, 2});
  const pluss::Series y = pluss::make_series(std::vector<std::int64_t>{0, 1, 2}, {3},
                                             pluss::Config{}.with_dtype(pluss::ScalarType::Int64));

  const pluss::Series predictions = model.forward(x);
  const pluss::Series loss = model.compute_loss(predictions, y, ice::LossKind::CrossEntropy);
  std::cout << "loss (label_smoothing=0.0) = " << loss.item<float>() << "\n";

  return 0;
}
```

Output

```text
loss (label_smoothing=0.0) = 1.21211
```

### Example 2

The same call, with `label_smoothing` -- a measurably different result,
confirming it actually changes the computed loss.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 3));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, {3, 2});
  const pluss::Series y = pluss::make_series(std::vector<std::int64_t>{0, 1, 2}, {3},
                                             pluss::Config{}.with_dtype(pluss::ScalarType::Int64));

  const pluss::Series predictions = model.forward(x);
  const pluss::Series loss = model.compute_loss(predictions, y, ice::LossKind::CrossEntropy, 0.2);
  std::cout << "loss (label_smoothing=0.2) = " << loss.item<float>() << "\n";

  return 0;
}
```

Output

```text
loss (label_smoothing=0.2) = 1.20477
```

### Example 3

The `Batch`/`StepContext` overload -- what every step method calls
internally.

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

  ice::StepContext ctx{};
  ctx.loss_kind = ice::LossKind::MSE;

  const pluss::Series predictions = model.forward(batch);
  const pluss::Series loss = model.compute_loss(predictions, batch, ctx);
  std::cout << "loss defined = " << loss.defined() << "\n";

  return 0;
}
```

Output

```text
loss defined = 1
```

## See Also

- [`forward`](forward.md) -- produces `predictions`.
- [`ObjectiveRegularization`](../../options/ObjectiveRegularization.md)
  -- `label_smoothing`/`l1_penalty`/`l2_penalty`, the fields the
  `Batch`/`StepContext` overload reads from `ctx.recipe`.
- [`learn_step`](learn_step.md)/[`valid_step`](valid_step.md)/
  [`test_step`](test_step.md) -- call the `Batch`/`StepContext`
  overload.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
