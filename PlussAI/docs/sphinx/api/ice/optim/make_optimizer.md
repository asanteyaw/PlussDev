# `pluss::ai::ice::make_optimizer`

Standalone optimizer construction (with clip/regularization pre-wired
from [`TrainOptions`](../options/TrainOptions.md)) for custom training
loops that do not fit [`Model`](../model/Model.md)'s generic
forward/loss shape -- for example a per-step loop with no fixed
`(x, y)` batch. `Coach` uses this exact function internally to build the
optimizer behind every `Model::learn` call; calling it directly gets
the identical construction (optimizer kind, learning rate, weight
decay, gradient clipping, non-finite diagnostics) without going through
`Model`/`Coach`/`Batch` at all.

## Signature

```cpp
[[nodiscard]] std::unique_ptr<pluss::ai::nn::optim::Optimizer> pluss::ai::ice::make_optimizer(
    const std::vector<pluss::Series*>& parameters, const pluss::ai::ice::TrainOptions& options);
```

## What It Does

Switches on `options.optimizer` and constructs the matching
`nn::optim::Optimizer` subclass (`Adam`, `AdamW`, `SGD`, `RMSprop`,
`Adagrad`, or `NAdam`) over `parameters`, with that algorithm's own
`Options` filled in from the parts of `TrainOptions` every optimizer
shares: `learning_rate`, `objective_regularization.weight_decay` as
that optimizer's own `weight_decay`, and
`execution.grad_clip` as that optimizer's own `clip` -- the same
`GradientClipOptions` [`ExecutionPolicy::grad_clip`](../options/ExecutionPolicy.md)
documents, on by default at clip-by-norm threshold `1.0`. For `AdamW`/
`NAdam` specifically, `objective_regularization.decoupled_weight_decay`
is also forwarded as that optimizer's own `decoupled_weight_decay`.

After construction, it calls `set_step_diagnostics` on the result with
`fail_if_non_finite_sensitivity`/`_parameter`/`_loss` all set from
`execution.fail_on_non_finite` -- the same non-finite safety net
[`ExecutionPolicy::fail_on_non_finite`](../options/ExecutionPolicy.md)
documents, applied identically whether training goes through `Model`
or through a hand-rolled loop built on this function directly.

## Parameters

`const std::vector<Series*>& parameters`
: The raw parameters to optimize, by pointer -- the same shape
  [`nn::optim`](../../nn/optim/Optimizer.md) constructors themselves
  take. Forwarded unchanged; `make_optimizer` does not discover
  parameters from a `Module` or `Model` itself, so collect whichever
  `Series` the custom loop actually updates.

`const TrainOptions& options`
: Selects the algorithm (`options.optimizer`) and its shared settings
  (`learning_rate`, `objective_regularization`, `execution.grad_clip`/
  `fail_on_non_finite`) exactly as [`Model::learn`](../model/Model/learn.md)
  would use the same `TrainOptions` value.

## Returns

`std::unique_ptr<nn::optim::Optimizer>` -- owning, so the caller holds
it for the lifetime of the custom loop. The dynamic type matches
`options.optimizer`, but every caller interacts with it only through
`Optimizer`'s own virtual surface (`step`, `parameters`, ...) --
nothing about `make_optimizer`'s return type requires knowing which
concrete class was selected.

## Errors

- Throws if `parameters` is empty.
- Throws if `objective_regularization.weight_decay`,
  `l1_penalty`, or `l2_penalty` is negative.
- Throws if `objective_regularization.decoupled_weight_decay` is set
  true while `options.optimizer` is not `AdamW` or `NAdam`.

## Practical Examples

Three examples: a direct hand-rolled step against the default (`Adam`),
the default gradient clip's effect on a plain `SGD` step, and a short
multi-step custom loop with no `Model`/`Batch` involved at all.

### Example 1

A minimal custom loop: no `Batch`, no `Source`, just raw parameters, an
ordinary `Series` expression for the loss, and one manual
`optimizer->step()` after `reverse()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  Series w = pluss::make_series({0.5F}, {1}, f32);
  Series b = pluss::make_series({0.0F}, {1}, f32);
  const Series x = pluss::make_series({2.0F}, {1}, f32);
  const Series target = pluss::make_series({5.0F}, {1}, f32);

  const ice::TrainOptions options{};  // Adam, learning_rate = 1e-3
  const auto optimizer = ice::make_optimizer({&w, &b}, options);

  std::cout << "before: w = " << w.item<float>() << ", b = " << b.item<float>() << "\n";

  const Series pred = w.mul(x).add(b);
  const Series diff = pred.sub(target);
  const Series loss = diff.mul(diff).mean();
  loss.reverse();
  optimizer->step();

  std::cout << "after:  w = " << w.item<float>() << ", b = " << b.item<float>() << "\n";

  return 0;
}
```

Output

```text
before: w = 0.5, b = 0
after:  w = 0.501, b = 0.00100001
```

### Example 2

`make_optimizer`'s whole point over constructing `SGD` directly is that
the clip/regularization in `options` is already wired in --
`execution.grad_clip` defaults to clip-by-norm at threshold `1.0`, which
shrinks a large raw gradient *before* `SGD`'s own update formula ever
sees it. Both runs below start from the identical `w = 0.5`, hit the
identical raw gradient, and differ only in whether `grad_clip` is left
at its default or switched off.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace optim = pluss::ai::nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = pluss::make_series({2.0F}, {1}, f32);
  const Series target = pluss::make_series({5.0F}, {1}, f32);

  Series w_clipped = pluss::make_series({0.5F}, {1}, f32);
  ice::TrainOptions clipped_options{};
  clipped_options.optimizer = ice::OptimizerKind::SGD;
  clipped_options.learning_rate = 0.5;
  const auto clipped_optimizer = ice::make_optimizer({&w_clipped}, clipped_options);
  {
    const Series diff = w_clipped.mul(x).sub(target);
    const Series loss = diff.mul(diff).mean();
    loss.reverse();
  }
  clipped_optimizer->step();
  std::cout << "default grad_clip (threshold 1.0): w after one step = " << w_clipped.item<float>() << "\n";

  Series w_unclipped = pluss::make_series({0.5F}, {1}, f32);
  ice::TrainOptions unclipped_options{};
  unclipped_options.optimizer = ice::OptimizerKind::SGD;
  unclipped_options.learning_rate = 0.5;
  unclipped_options.execution.grad_clip.mode = optim::GradientClipMode::None;
  const auto unclipped_optimizer = ice::make_optimizer({&w_unclipped}, unclipped_options);
  {
    const Series diff = w_unclipped.mul(x).sub(target);
    const Series loss = diff.mul(diff).mean();
    loss.reverse();
  }
  unclipped_optimizer->step();
  std::cout << "grad_clip disabled: w after one step = " << w_unclipped.item<float>() << "\n";

  return 0;
}
```

Output

```text
default grad_clip (threshold 1.0): w after one step = 1
grad_clip disabled: w after one step = 8.5
```

### Example 3

A short end-to-end loop over several steps -- the realistic shape
`make_optimizer` exists for: a model-free `(w, b)` pair trained toward
a target with plain `Series` arithmetic, `Adam` at a chosen
`learning_rate`, and nothing from `Model`/`Batch`/`Source` anywhere in
the loop.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  Series w = pluss::make_series({0.5F}, {1}, f32);
  Series b = pluss::make_series({0.0F}, {1}, f32);
  const Series x = pluss::make_series({2.0F}, {1}, f32);
  const Series target = pluss::make_series({5.0F}, {1}, f32);

  ice::TrainOptions options{};
  options.learning_rate = 0.05;
  const auto optimizer = ice::make_optimizer({&w, &b}, options);

  for (int step = 0; step < 5; ++step) {
    const Series diff = w.mul(x).add(b).sub(target);
    const Series loss = diff.mul(diff).mean();
    loss.reverse();
    optimizer->step();
    std::cout << "step " << step << ": loss = " << loss.item<float>()
               << ", w = " << w.item<float>() << ", b = " << b.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: loss = 16, w = 0.55, b = 0.0500003
step 1: loss = 14.8225, w = 0.600001, b = 0.100001
step 2: loss = 13.69, w = 0.650001, b = 0.150001
step 3: loss = 12.6025, w = 0.700001, b = 0.200001
step 4: loss = 11.56, w = 0.750002, b = 0.250002
```

## See Also

- [`TrainOptions`](../options/TrainOptions.md) -- every field this
  function reads.
- [`ExecutionPolicy::grad_clip`](../options/ExecutionPolicy.md) /
  [`fail_on_non_finite`](../options/ExecutionPolicy.md) -- the clip and
  non-finite diagnostics pre-wired into the result.
- [`Optimizer`](../../nn/optim/Optimizer.md) -- the shared base class
  of everything this function can return.
- [`Coach`](../coach/Coach.md) -- calls this exact function internally
  to build the optimizer behind every `Model::learn`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
