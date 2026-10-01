# `pluss::ai::ice::StepContext`

A plain, read-only context struct: the per-step bookkeeping the training
loop passes into every [`Model`](../model/Model.md) step method
([`learn_step`](../model/Model.md)/[`valid_step`](../model/Model.md)/
[`test_step`](../model/Model.md)/[`infer_step`](../model/Model.md)) and
into [`compute_loss(predictions, batch, ctx)`](../model/Model/compute_loss.md).
Never constructed by calling code in the ordinary `learn`/`infer`/
`assess` flow -- [`Coach::learn`](../coach/Coach.md) builds one per step
internally; a caller only constructs one by hand when calling a step
method directly, outside that loop.

## Signature

```cpp
pluss::ai::ice::StepContext::StepContext() = default;
```

A plain aggregate -- every field below defaults to a harmless empty/
sentinel value (`recipe = nullptr`, `stage = StepStage::Infer`, `epoch =
-1`, `batch_index = -1`), but a default-constructed instance only
matters as a starting point for setting the fields a direct step-method
call needs.

## State

### `recipe`

`const TrainOptions* recipe = nullptr`
: The [`TrainOptions`](../options/TrainOptions.md) this step is running
  under -- `nullptr` outside a `Coach`-driven loop.
  [`compute_loss`](../model/Model/compute_loss.md) reads
  `recipe->objective_regularization` from this (when non-null) to apply
  L1/L2 penalties and label smoothing.

### `loss_kind`

`std::optional<LossKind> loss_kind = std::nullopt`
: Which loss to compute -- required (checked, throws if unset) by
  [`compute_loss(predictions, batch, ctx)`](../model/Model/compute_loss.md).
  Unused by `infer_step`, which never computes a loss.

### `stage`

`StepStage stage = StepStage::Infer`
: Which phase this step belongs to: `Learn`, `Valid`, `Test`, or
  `Infer`. A `Model` override that behaves differently per phase (for
  example, skipping a regularization term outside `Learn`) reads this.

### `epoch`

`std::int64_t epoch = -1`
: The current epoch number, `-1` outside a training loop.

### `batch_index`

`std::int64_t batch_index = -1`
: The current batch's index within this epoch, `-1` outside a training
  loop.

## Methods

None -- `StepContext` is a plain read-only context; every field is
public and read directly, see `## State` above.

## Practical Examples

One example: a custom `Model` subclass overriding `learn_step` to log
`stage`/`epoch`/`batch_index` as `Coach::learn` drives real training.

### Example 1

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

`stage=0` is `StepStage::Learn` -- two batches per epoch (4 rows at
`batch_size = 2`), across the two requested epochs.

## Changelog

- **0.1** — `recipe`/`loss_kind`/`stage`/`epoch`/`batch_index`, the
  per-step bookkeeping passed into every `Model` step method.

## See Also

- [`Model`](../model/Model.md) -- the step methods this is passed into.
- [`Batch`](Batch.md) -- passed alongside this to every step method.
- [`StepResult`](StepResult.md) -- what a step method returns.
- [`Coach::learn`](../coach/Coach.md) -- constructs one per step
  internally.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
