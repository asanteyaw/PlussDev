<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8261aa53d988 -->
# `pluss::ai::ice::Model::learn_step`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::ai::ice::StepResult pluss::ai::ice::Model::learn_step(const pluss::ai::ice::Batch& batch, const pluss::ai::ice::StepContext& ctx) const;
```

## What It Does

The per-batch training step: calls `forward(batch)`, then
`compute_loss(predictions, batch, ctx)`, and returns both. [`Coach::learn`](../../coach/Coach.md)
calls this once per training batch and runs the optimizer's `step()`
against the returned `loss`. Override this directly only to change
training-time behavior specifically, leaving `valid_step`/`test_step`
(which otherwise share the identical default body) unaffected.

## Parameters

`const Batch& batch`
: The training batch -- must have both `inputs` and `targets` set.

`const StepContext& ctx`
: `ctx.loss_kind` must be set; `ctx.stage` is `StepStage::Learn` when
  called from `Coach::learn`.

## Returns

[`StepResult`](../../step/StepResult.md) -- `predictions` and `loss`,
both defined.

## Errors

Same as [`compute_loss`](compute_loss.md)'s `Batch`/`StepContext`
overload: throws if `batch` has no `targets`, or `ctx.loss_kind` is
unset.

## Practical Examples

Three examples: a direct call, reading the result's two fields, and a
custom override that logs before delegating to the default.

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

  const ice::StepResult result = model.learn_step(batch, ctx);
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

`predictions`' shape matches the model's own output shape.

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

  const ice::StepResult result = model.learn_step(batch, ctx);
  std::cout << "predictions shape = [" << result.predictions.sizes()[0] << ","
            << result.predictions.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
predictions shape = [1,1]
```

### Example 3

A custom `Model` subclass overriding `learn_step` to log per-batch
context, delegating to `Model::learn_step` for the actual work -- driven
end to end by `Coach::learn` (through `Model::learn`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

class LoggingModel : public ice::Model {
 public:
  LoggingModel() : head_(2, 1) { register_module(*head_.get()); }

  pluss::Series forward(const pluss::Series& x) const override { return head_->forward(x); }

  ice::StepResult learn_step(const ice::Batch& batch, const ice::StepContext& ctx) const override {
    std::cout << "learn_step: stage=" << static_cast<int>(ctx.stage) << " epoch=" << ctx.epoch
              << " batch_index=" << ctx.batch_index << "\n";
    return ice::Model::learn_step(batch, ctx);
  }

 private:
  nn::Fusion head_;
};

int main() {
  LoggingModel model;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 2;
  options.batch_size = 2;
  options.shuffle = false;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  return 0;
}
```

Output

```text
learn_step: stage=0 epoch=0 batch_index=0
learn_step: stage=0 epoch=0 batch_index=1
learn_step: stage=0 epoch=1 batch_index=0
learn_step: stage=0 epoch=1 batch_index=1
```

## See Also

- [`valid_step`](valid_step.md) / [`test_step`](test_step.md) -- the
  same default body, for the other two stages.
- [`infer_step`](infer_step.md) -- the one step method that never
  computes a loss.
- [`compute_loss`](compute_loss.md) -- what this calls after `forward`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
