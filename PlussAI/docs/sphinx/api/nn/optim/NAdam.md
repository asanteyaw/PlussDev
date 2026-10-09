# `pluss::ai::nn::optim::NAdam`

`Adam`'s bias-corrected moment estimation, with Nesterov-style momentum
folded directly into the first-moment update -- the same lookahead idea
[`SGDOptions::nesterov`](SGDOptions.md) applies to plain momentum, adapted
for Adam's moment-based formula. A `momentum_decay` schedule additionally
governs how the momentum coefficient itself grows over training steps,
distinct from `beta1`. `NAdam` also has its own `decoupled_weight_decay`
toggle -- unlike [`Adam`](Adam.md)/[`AdamW`](AdamW.md), which pick one
weight-decay mechanism each, `NAdam` supports both from a single type (see
Example 2). Every method not specific to `NAdam`'s own formula is
inherited unchanged from [`Optimizer`](Optimizer.md).

## Signature

```cpp
explicit nn::optim::NAdam::NAdam(pluss::RangeView<pluss::Series*> parameters, nn::optim::NAdamOptions options = {});
explicit nn::optim::NAdam::NAdam(pluss::RangeView<nn::optim::NAdamGroup> groups);
explicit nn::optim::NAdam::NAdam(nn::Module& module, nn::optim::NAdamOptions options = {});

const nn::optim::NAdamOptions& nn::optim::NAdam::options() const noexcept;
void nn::optim::NAdam::set_options(const nn::optim::NAdamOptions& options);
void nn::optim::NAdam::set_group_options(std::size_t group_index, const nn::optim::NAdamOptions& options);
std::size_t nn::optim::NAdam::group_count() const noexcept;

nn::optim::NAdam::State nn::optim::NAdam::capture_state() const;
void nn::optim::NAdam::restore_state(const nn::optim::NAdam::State& state);
```

`State` carries the same moment-estimate buffers as [`Adam`](Adam.md)'s
own, plus the accumulated momentum-schedule product `NAdam`'s own formula
needs.

## Construction

Same three-overload shape as [`Adam`](Adam.md): an explicit parameter
list plus shared [`NAdamOptions`](NAdamOptions.md), a per-group
[`NAdamGroup`](NAdamGroup.md) list, or a `Module&`.

## State

Same shape as [`Adam`](Adam.md)'s own `## State`.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Three examples: `NAdam` against plain `Adam` under identical
hyperparameters, `decoupled_weight_decay` toggled within `NAdam` itself,
and an end-to-end `Module` training loop.

### Example 1

Same starting point, target, and learning rate as [`Adam`](Adam.md)'s own
Example 1 -- `NAdam`'s lookahead momentum produces a visibly different
trajectory even with every shared hyperparameter identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series target = pluss::make_series(1.0F, f32);

  Series w_adam = pluss::make_series(5.0F, f32);
  optim::Adam adam(std::vector<Series*>{&w_adam}, optim::AdamOptions{.learning_rate = 0.5});

  Series w_nadam = pluss::make_series(5.0F, f32);
  optim::NAdam nadam(std::vector<Series*>{&w_nadam}, optim::NAdamOptions{.learning_rate = 0.5});

  for (int step = 0; step < 8; ++step) {
    const Series w_adam_residual = w_adam.sub(target);
    const float w_adam_loss = w_adam_residual.square().mul(0.5).item<float>();
    adam.step(std::vector<Series>{w_adam_residual});
    const Series w_nadam_residual = w_nadam.sub(target);
    const float w_nadam_loss = w_nadam_residual.square().mul(0.5).item<float>();
    nadam.step(std::vector<Series>{w_nadam_residual});
    std::cout << "step " << step << ": Adam loss = " << w_adam_loss << ", NAdam loss = " << w_nadam_loss << ", Adam w = " << w_adam.item<float>()
               << ", NAdam w = " << w_nadam.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: Adam loss = 8, NAdam loss = 8, Adam w = 4.5, NAdam w = 4.47177
step 1: Adam loss = 6.125, NAdam loss = 6.0266, Adam w = 4.00284, NAdam w = 4.10538
step 2: Adam loss = 4.50852, NAdam loss = 4.82169, Adam w = 3.5111, NAdam w = 3.77764
step 3: Adam loss = 3.1528, NAdam loss = 3.85764, Adam w = 3.028, NAdam w = 3.46436
step 4: Adam loss = 2.05638, NAdam loss = 3.03653, Adam w = 2.55751, NAdam w = 3.15983
step 5: Adam loss = 1.21293, NAdam loss = 2.33243, Adam w = 2.10445, NAdam w = 2.86408
step 6: Adam loss = 0.609909, NAdam loss = 1.7374, Adam w = 1.67443, NAdam w = 2.57922
step 7: Adam loss = 0.227427, NAdam loss = 1.24697, Adam w = 1.27372, NAdam w = 2.30805
```

NAdam is more conservative here -- its own default `learning_rate` (`2e-3`
in [`NAdamOptions`](NAdamOptions.md), versus `1e-3` for `AdamOptions`) is
tuned assuming its own momentum schedule, not `Adam`'s; this example
overrides both to the same value specifically to isolate the momentum
formula's own effect.

### Example 2

`weight_decay` with no real gradient signal (always `0`), comparing
`NAdam`'s own coupled and decoupled modes directly -- the same isolation
[`AdamW`](AdamW.md) Example 2 demonstrates across two different optimizer
types, reproduced here within one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series zero_grad = pluss::make_series(0.0F, f32);

  Series w_coupled = pluss::make_series(10.0F, f32);
  optim::NAdam coupled(std::vector<Series*>{&w_coupled},
                       optim::NAdamOptions{.learning_rate = 0.1, .weight_decay = 0.1, .decoupled_weight_decay = false});

  Series w_decoupled = pluss::make_series(10.0F, f32);
  optim::NAdam decoupled(std::vector<Series*>{&w_decoupled},
                         optim::NAdamOptions{.learning_rate = 0.1, .weight_decay = 0.1, .decoupled_weight_decay = true});

  for (int step = 0; step < 5; ++step) {
    coupled.step(std::vector<Series>{zero_grad});
    decoupled.step(std::vector<Series>{zero_grad});
    std::cout << "step " << step << ": coupled w = " << w_coupled.item<float>()
               << ", decoupled w = " << w_decoupled.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: coupled w = 9.89435, decoupled w = 9.9
step 1: coupled w = 9.81635, decoupled w = 9.801
step 2: coupled w = 9.74368, decoupled w = 9.70299
step 3: coupled w = 9.67129, decoupled w = 9.60596
step 4: coupled w = 9.5976, decoupled w = 9.5099
```

`decoupled_weight_decay = true` again produces the exact geometric decay
`10 * (1 - 0.1*0.1)^t`, same as [`AdamW`](AdamW.md) Example 2's decoupled
column, independent of everything else the optimizer is doing.

### Example 3

An end-to-end training loop via the closure `step` overload.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  nn::Fusion model(2, 1, /*bias=*/false, f32);
  const Series w = pluss::make_series({0.5F, -0.5F}, {1, 2}, f32);
  model->weight().copy_(w);

  const Series x = pluss::make_series({1.0F, 1.0F}, {1, 2}, f32);
  const Series y = pluss::make_series({2.0F}, {1, 1}, f32);

  optim::NAdam optimizer(*model, optim::NAdamOptions{.learning_rate = 0.2});

  for (int epoch = 0; epoch < 5; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      const Series prediction = model.forward(x);
      return nn::losses::MSE(prediction, y);
    });
    std::cout << "epoch " << epoch << ": loss = " << loss.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
epoch 0: loss = 4
epoch 1: loss = 2.48824
epoch 2: loss = 1.68657
epoch 3: loss = 1.12128
epoch 4: loss = 0.705289
```

## Changelog

- **0.1** — `Adam`'s moment estimation with Nesterov momentum folded into
  the first-moment update, a `momentum_decay` schedule, and a choice of
  coupled or decoupled weight decay.

## See Also

- [`NAdamOptions`](NAdamOptions.md) -- every field's default and effect.
- [`NAdamGroup`](NAdamGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`Adam`](Adam.md) -- the moment estimation this is built on, without the
  Nesterov correction.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

NAdamGroup
NAdamOptions
```
<!-- apiref:toctree:end -->
