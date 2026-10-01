<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 554478aca145 -->
# `pluss::ai::ice::StepResult::has_loss`

[Back to `pluss::ai::ice::StepResult`](../StepResult.md)

## Signature

```cpp
bool pluss::ai::ice::StepResult::has_loss() const noexcept;
```

## What It Does

Equivalent to `loss.defined()`. `learn_step`/`valid_step`/`test_step`
always set `loss`; `infer_step` never does (there is no target to
compute a loss against during plain inference), so this is the way to
tell the two cases apart from the result alone.

## Parameters

None.

## Returns

`bool` -- `true` if `loss` is defined.

## Errors

None. `has_loss` is `noexcept` and never throws.

## Practical Examples

One example: `learn_step` sets `loss`, `infer_step` does not.

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

  std::cout << "learn_step has_loss() = " << learn_result.has_loss() << "\n";
  std::cout << "infer_step has_loss() = " << infer_result.has_loss() << "\n";

  return 0;
}
```

Output

```text
learn_step has_loss() = 1
infer_step has_loss() = 0
```

### Example 2

`valid_step` also always sets `loss`.

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
  std::cout << "valid_step has_loss() = " << result.has_loss() << "\n";

  return 0;
}
```

Output

```text
valid_step has_loss() = 1
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
  std::cout << "test_step has_loss() = " << result.has_loss() << "\n";

  return 0;
}
```

Output

```text
test_step has_loss() = 1
```

## See Also

- [`has_predictions`](has_predictions.md) -- the other field, which
  every step method sets.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="has_predictions.html">
    <span class="label">Next</span>
    <span class="title">has_predictions</span>
  </a>
</div>
<!-- apiref:nav:end -->
