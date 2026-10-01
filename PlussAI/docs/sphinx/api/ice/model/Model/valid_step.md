# `pluss::ai::ice::Model::valid_step`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::ai::ice::StepResult pluss::ai::ice::Model::valid_step(const pluss::ai::ice::Batch& batch, const pluss::ai::ice::StepContext& ctx) const;
```

## What It Does

The per-batch validation step -- identical default body to
[`learn_step`](learn_step.md) (`forward` then `compute_loss`), called
by [`Coach::learn`](../../coach/Coach.md) during the validation pass
instead of training. No optimizer step runs against its result; the
returned `loss` only feeds
[`ValidationPolicy::monitor`](../../options/ValidationPolicy.md)/
early stopping. Override this specifically (leaving `learn_step`
unaffected) for a model whose validation-time behavior should differ
from training -- for example, skipping `objective_regularization`'s
penalties, which `compute_loss`'s `Batch`/`StepContext` overload applies
unconditionally by default regardless of `ctx.stage`.

## Parameters

`const Batch& batch`
: The validation batch -- must have both `inputs` and `targets` set.

`const StepContext& ctx`
: `ctx.loss_kind` must be set; `ctx.stage` is `StepStage::Valid` when
  called from `Coach::learn`.

## Returns

[`StepResult`](../../step/StepResult.md) -- `predictions` and `loss`,
both defined.

## Errors

Same as [`learn_step`](learn_step.md): throws if `batch` has no
`targets`, or `ctx.loss_kind` is unset.

## Practical Examples

Three examples: a direct call, confirming it matches `learn_step`'s
output for the same inputs, and reading `loss` back.

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

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {1, 2}));
  batch.set_targets(pluss::make_series({3.0f}, {1, 1}));

  ice::StepContext ctx{};
  ctx.loss_kind = ice::LossKind::MSE;

  const ice::StepResult result = model.valid_step(batch, ctx);
  std::cout << "has_predictions() = " << result.has_predictions() << "\n";
  std::cout << "has_loss() = " << result.has_loss() << "\n";

  return 0;
}
```

Output

```text
has_predictions() = 1
has_loss() = 1
```

### Example 2

With the model in a fixed (untrained) state, `valid_step` and
`learn_step` produce the identical result for the same batch -- neither
runs an optimizer step itself, only `Coach::learn`'s own loop does that
afterward, against `learn_step`'s result specifically.

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

  const ice::StepResult learn_result = model.learn_step(batch, ctx);
  const ice::StepResult valid_result = model.valid_step(batch, ctx);

  const double diff = learn_result.loss.sub(valid_result.loss).abs().amax().item<double>();
  std::cout << "loss diff = " << diff << "\n";

  return 0;
}
```

Output

```text
loss diff = 0
```

### Example 3

Reading the loss value back directly.

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

  const ice::StepResult result = model.valid_step(batch, ctx);
  std::cout << "loss.defined() = " << result.loss.defined() << "\n";

  return 0;
}
```

Output

```text
loss.defined() = 1
```

## See Also

- [`learn_step`](learn_step.md) -- the identical default body, called
  during training instead.
- [`test_step`](test_step.md) -- the same default body again, for the
  test stage.
- [`Coach::learn`](../../coach/Coach.md) -- calls this during its
  validation pass.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
