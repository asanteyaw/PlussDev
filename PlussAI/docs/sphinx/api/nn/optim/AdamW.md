# `pluss::ai::nn::optim::AdamW`

`Adam`'s bias-corrected first/second moment estimation, with one change:
weight decay is *decoupled* -- applied as a direct multiplicative shrink
on the parameter (`param *= 1 - learning_rate * weight_decay`) before the
moment-based update, rather than folded into the gradient the way
[`Adam`](Adam.md)'s own `weight_decay` field is. This is the fix the
"Decoupled Weight Decay Regularization" paper introduced: coupling decay
into the gradient means it gets scaled by the same adaptive per-parameter
factor as the real gradient signal, which was never the intent of weight
decay as regularization. `AdamW` also defaults `weight_decay` to `1e-2`
rather than `Adam`'s `0.0` -- decoupled decay is expected to be turned on,
not opted into. Every method not specific to `AdamW`'s own formula is
inherited unchanged from [`Optimizer`](Optimizer.md).

## Signature

```cpp
explicit nn::optim::AdamW::AdamW(pluss::RangeView<pluss::Series*> parameters, nn::optim::AdamWOptions options = {});
explicit nn::optim::AdamW::AdamW(pluss::RangeView<nn::optim::AdamWGroup> groups);
explicit nn::optim::AdamW::AdamW(nn::Module& module, nn::optim::AdamWOptions options = {});

const nn::optim::AdamWOptions& nn::optim::AdamW::options() const noexcept;
void nn::optim::AdamW::set_options(const nn::optim::AdamWOptions& options);
void nn::optim::AdamW::set_group_options(std::size_t group_index, const nn::optim::AdamWOptions& options);
std::size_t nn::optim::AdamW::group_count() const noexcept;

nn::optim::AdamW::State nn::optim::AdamW::capture_state() const;
void nn::optim::AdamW::restore_state(const nn::optim::AdamW::State& state);
```

`State` has the exact same shape as [`Adam`](Adam.md)'s own -- step
counter, each group's options, and every parameter's moment estimates.

## Construction

Same three-overload shape as [`Adam`](Adam.md): an explicit parameter
list plus shared [`AdamWOptions`](AdamWOptions.md), a per-group
[`AdamWGroup`](AdamWGroup.md) list, or a `Module&`.

## State

Same shape as [`Adam`](Adam.md)'s own `## State`.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Three examples: baseline convergence under `AdamW`'s own defaults (which
already differ from `Adam`'s, since `weight_decay` defaults to `1e-2` here
rather than `0.0`), a direct comparison isolating decoupled decay's effect
against `Adam`'s coupled decay, and an end-to-end `Module` training loop.

### Example 1

The same starting point and learning rate as [`Adam`](Adam.md)'s own
Example 1, but with `AdamWOptions{}`'s default `weight_decay = 1e-2` left
in place (`Adam`'s own default is `0.0`) -- the trajectory differs
slightly from `Adam`'s Example 1 purely because of that default, with no
option overridden to demonstrate it.

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

  Series w = pluss::make_series(5.0F, f32);
  optim::AdamW optimizer(std::vector<Series*>{&w}, optim::AdamWOptions{.learning_rate = 0.5});

  for (int step = 0; step < 8; ++step) {
    optimizer.step(std::vector<Series>{w.sub(target)});
    std::cout << "step " << step << ": w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: w = 4.475
step 1: w = 3.95567
step 2: w = 3.44479
step 3: w = 2.94578
step 4: w = 2.46292
step 5: w = 2.00132
step 6: w = 1.56691
step 7: w = 1.16623
```

### Example 2

A parameter with **no real gradient signal at all** (the supplied gradient
is always `0`) -- only `weight_decay` can move it. `Adam`'s coupled decay
folds `weight_decay * parameter` into the (otherwise-zero) gradient and
routes it through the adaptive moment machinery; `AdamW`'s decoupled decay
shrinks the parameter by a fixed multiplicative factor directly. Both
still shrink the parameter toward zero here, but through genuinely
different mechanisms -- `AdamW`'s trajectory is the exact geometric decay
`10 * (1 - 0.1*0.1)^t = 10 * 0.99^t`; `Adam`'s is not.

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

  Series w_adam = pluss::make_series(10.0F, f32);
  optim::Adam adam(std::vector<Series*>{&w_adam}, optim::AdamOptions{.learning_rate = 0.1, .weight_decay = 0.1});

  Series w_adamw = pluss::make_series(10.0F, f32);
  optim::AdamW adamw(std::vector<Series*>{&w_adamw}, optim::AdamWOptions{.learning_rate = 0.1, .weight_decay = 0.1});

  for (int step = 0; step < 5; ++step) {
    adam.step(std::vector<Series>{zero_grad});
    adamw.step(std::vector<Series>{zero_grad});
    std::cout << "step " << step << ": Adam (coupled) w = " << w_adam.item<float>()
               << ", AdamW (decoupled) w = " << w_adamw.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: Adam (coupled) w = 9.9, AdamW (decoupled) w = 9.9
step 1: Adam (coupled) w = 9.80003, AdamW (decoupled) w = 9.801
step 2: Adam (coupled) w = 9.7001, AdamW (decoupled) w = 9.70299
step 3: Adam (coupled) w = 9.60024, AdamW (decoupled) w = 9.60596
step 4: Adam (coupled) w = 9.50046, AdamW (decoupled) w = 9.5099
```

### Example 3

An end-to-end training loop via the closure `step` overload -- identical
shape to [`Adam`](Adam.md)'s own Example 3, with `AdamW` in place of
`Adam`.

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

  optim::AdamW optimizer(*model, optim::AdamWOptions{.learning_rate = 0.2});

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
epoch 1: loss = 2.55999
epoch 2: loss = 1.45334
epoch 3: loss = 0.675422
epoch 4: loss = 0.208165
```

## Changelog

- **0.1** — `Adam`'s moment estimation with decoupled weight decay,
  defaulting `weight_decay` to `1e-2`.

## See Also

- [`AdamWOptions`](AdamWOptions.md) -- every field's default and effect.
- [`AdamWGroup`](AdamWGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`Adam`](Adam.md) -- the coupled-decay counterpart this is built on.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

AdamWGroup
AdamWOptions
```
<!-- apiref:toctree:end -->
