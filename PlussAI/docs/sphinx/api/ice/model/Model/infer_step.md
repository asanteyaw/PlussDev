# `pluss::ai::ice::Model::infer_step`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::ai::ice::StepResult pluss::ai::ice::Model::infer_step(const pluss::ai::ice::Batch& batch, const pluss::ai::ice::StepContext& ctx) const;
```

## What It Does

The per-batch inference step -- the one step method that never computes
a loss: its default calls `forward(batch)` and returns
`StepResult{.predictions = ..., .loss = undefined}`. `ctx` is accepted
for a consistent signature across all four step methods but is unused
by the default implementation (there is no target to compute a loss
against during plain inference). [`Coach::infer`](../../coach/Coach.md)
calls this once per batch.

## Parameters

`const Batch& batch`
: The input batch -- must have `inputs` set; `targets` is not required
  (and, if present, is ignored).

`const StepContext& ctx`
: Unused by the default implementation; `ctx.stage` is
  `StepStage::Infer` when called from `Coach::infer`.

## Returns

[`StepResult`](../../step/StepResult.md) -- `predictions` defined,
`loss` undefined.

## Errors

- `batch` has no `inputs` field set -- throws (the same error
  [`forward(const Batch&)`](forward.md) throws).

## Practical Examples

Three examples: a direct call showing `loss` stays undefined, a batch
with no `targets` at all (still works, since `infer_step` never needs
one), and the error from a batch missing `inputs`.

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
  const ice::StepResult result = model.infer_step(batch, ctx);
  std::cout << "has_predictions() = " << result.has_predictions() << "\n";
  std::cout << "has_loss() = " << result.has_loss() << "\n";

  return 0;
}
```

Output

```text
has_predictions() = 1
has_loss() = 0
```

### Example 2

No `targets` at all -- `infer_step` works regardless, unlike the other
three step methods.

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

  ice::StepContext ctx{};
  const ice::StepResult result = model.infer_step(batch, ctx);
  std::cout << "has_predictions() = " << result.has_predictions() << "\n";

  return 0;
}
```

Output

```text
has_predictions() = 1
```

### Example 3

A batch with no `inputs` set throws -- the same error `forward(const
Batch&)` throws, since `infer_step`'s default calls it directly.

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
  ice::StepContext ctx{};
  try {
    model.infer_step(batch, ctx);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "threw: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
threw: pluss::ai::ice::Model::forward(batch): batch.inputs must be defined.
```

## See Also

- [`learn_step`](learn_step.md)/[`valid_step`](valid_step.md)/
  [`test_step`](test_step.md) -- the three step methods that do compute
  a loss.
- [`forward`](forward.md) -- what this calls directly.
- [`Coach::infer`](../../coach/Coach.md) -- calls this once per batch.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
