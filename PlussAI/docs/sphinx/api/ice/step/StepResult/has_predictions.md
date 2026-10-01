<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7529eb1d8dbd -->
# `pluss::ai::ice::StepResult::has_predictions`

[Back to `pluss::ai::ice::StepResult`](../StepResult.md)

## Signature

```cpp
bool pluss::ai::ice::StepResult::has_predictions() const noexcept;
```

## What It Does

Equivalent to `predictions.defined()`. All four built-in step methods
(`learn_step`/`valid_step`/`test_step`/`infer_step`) always set
`predictions`, so this is `true` for any result they produce; it exists
mainly for a custom override that might legitimately leave it undefined.

## Parameters

None.

## Returns

`bool` -- `true` if `predictions` is defined.

## Errors

None. `has_predictions` is `noexcept` and never throws.

## Practical Examples

One example: reading `has_predictions` off both `learn_step` and
`infer_step`'s results -- both set it, even though only `learn_step`
also sets `loss`.

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

  const ice::StepResult learn_result = model.learn_step(batch, ctx);
  const ice::StepResult infer_result = model.infer_step(batch, ctx);

  std::cout << "learn_step has_predictions() = " << learn_result.has_predictions() << "\n";
  std::cout << "infer_step has_predictions() = " << infer_result.has_predictions() << "\n";

  return 0;
}
```

Output

```text
learn_step has_predictions() = 1
infer_step has_predictions() = 1
```

### Example 2

`valid_step` also always sets `predictions`.

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
  std::cout << "valid_step has_predictions() = " << result.has_predictions() << "\n";

  return 0;
}
```

Output

```text
valid_step has_predictions() = 1
```

### Example 3

And so does `test_step`.

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

  const ice::StepResult result = model.test_step(batch, ctx);
  std::cout << "test_step has_predictions() = " << result.has_predictions() << "\n";

  return 0;
}
```

Output

```text
test_step has_predictions() = 1
```

## See Also

- [`has_loss`](has_loss.md) -- the other field, which `infer_step`
  leaves undefined.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_loss.html">
    <span class="label">Previous</span>
    <span class="title">has_loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
