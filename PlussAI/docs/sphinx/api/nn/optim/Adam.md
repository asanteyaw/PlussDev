# `pluss::ai::nn::optim::Adam`

Adaptive moment estimation: each parameter gets its own running estimate
of the gradient's first moment (a momentum-like average of recent
gradients) and second moment (a decayed running average of recent squared
gradients), both bias-corrected for the fact that they start at zero. The
update divides the (bias-corrected) first moment by the square root of the
(bias-corrected) second moment, so a parameter's effective step size
adapts to how large and how consistent its own recent gradients have
been -- the default choice for most models in this reference, and the one
[`AdamW`](AdamW.md)/[`NAdam`](NAdam.md) are both built directly on top of.
Every method not specific to `Adam`'s own formula is inherited unchanged
from [`Optimizer`](Optimizer.md).

## Signature

```cpp
explicit nn::optim::Adam::Adam(pluss::RangeView<pluss::Series*> parameters, nn::optim::AdamOptions options = {});
explicit nn::optim::Adam::Adam(pluss::RangeView<nn::optim::AdamGroup> groups);
explicit nn::optim::Adam::Adam(nn::Module& module, nn::optim::AdamOptions options = {});

const nn::optim::AdamOptions& nn::optim::Adam::options() const noexcept;
void nn::optim::Adam::set_options(const nn::optim::AdamOptions& options);
void nn::optim::Adam::set_group_options(std::size_t group_index, const nn::optim::AdamOptions& options);
std::size_t nn::optim::Adam::group_count() const noexcept;

nn::optim::Adam::State nn::optim::Adam::capture_state() const;
void nn::optim::Adam::restore_state(const nn::optim::Adam::State& state);
```

`State` is a plain aggregate: the global step counter, each group's own
options, and every parameter's first moment, second moment, and (when
`amsgrad` is on) the running maximum second moment -- everything the
formula needs to resume exactly where it left off.

## Construction

Same three-overload shape as [`SGD`](SGD.md): an explicit parameter list
plus shared [`AdamOptions`](AdamOptions.md), a per-group
[`AdamGroup`](AdamGroup.md) list, or a `Module&`.

```cpp
nn::optim::Adam a(std::vector<Series*>{&w}, nn::optim::AdamOptions{.learning_rate = 1e-3});
nn::optim::Adam b(*model, nn::optim::AdamOptions{.learning_rate = 1e-3});
```

## State

Same shape as [`SGD`](SGD.md)'s own `## State`: `options()`/
`set_options()`/`set_group_options()`/`group_count()` read or replace
[`AdamOptions`](AdamOptions.md), and `capture_state()`/`restore_state()`
snapshot Adam's own internal state -- the step counter and every
parameter's moment estimates -- independent of the parameter's own value.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Four examples: baseline convergence on a simple quadratic, `amsgrad`'s
effect after a gradient spike, an end-to-end `Module` training loop, and
that same loop run on MPS instead of CPU.

### Example 1

A single parameter converging toward a target under `Adam`'s default
betas (`0.9`/`0.999`) -- the step size ramps up over the first few calls
as the bias correction's effect fades, then settles into steady descent.

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
  optim::Adam optimizer(std::vector<Series*>{&w}, optim::AdamOptions{.learning_rate = 0.5});

  for (int step = 0; step < 8; ++step) {
    const Series w_residual = w.sub(target);
    const float w_loss = w_residual.square().mul(0.5).item<float>();
    optimizer.step(std::vector<Series>{w_residual});
    std::cout << "step " << step << ": loss = " << w_loss << ", w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: loss = 8, w = 4.5
step 1: loss = 6.125, w = 4.00284
step 2: loss = 4.50852, w = 3.5111
step 3: loss = 3.1528, w = 3.028
step 4: loss = 2.05638, w = 2.55751
step 5: loss = 1.21293, w = 2.10445
step 6: loss = 0.609909, w = 1.67443
step 7: loss = 0.227427, w = 1.27372
```

### Example 2

A synthetic gradient sequence -- one large spike (`20.0`), then several
small, constant gradients (`0.1`) -- shows `amsgrad`'s actual effect.
Plain `Adam`'s second-moment estimate decays after the spike passes, so
its effective step size grows back; `amsgrad = true` keeps the *maximum*
second moment ever observed, permanently suppressing the step size after
a large gradient, even once gradients shrink back down.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const std::vector<float> grads = {20.0F, 0.1F, 0.1F, 0.1F, 0.1F, 0.1F, 0.1F};

  Series w_plain = pluss::make_series(0.0F, f32);
  optim::Adam plain(std::vector<Series*>{&w_plain}, optim::AdamOptions{.learning_rate = 0.1});

  Series w_ams = pluss::make_series(0.0F, f32);
  optim::Adam amsgrad(std::vector<Series*>{&w_ams}, optim::AdamOptions{.learning_rate = 0.1, .amsgrad = true});

  for (std::size_t step = 0; step < grads.size(); ++step) {
    const Series g = pluss::make_series(grads[step], f32);
    plain.step(std::vector<Series>{g});
    amsgrad.step(std::vector<Series>{g});
    std::cout << "step " << step << " (grad=" << grads[step] << "): plain Adam w = " << w_plain.item<float>()
               << ", amsgrad w = " << w_ams.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0 (grad=20): plain Adam w = -0.100001, amsgrad w = -0.100001
step 1 (grad=0.1): plain Adam w = -0.167378, amsgrad w = -0.147633
step 2 (grad=0.1): plain Adam w = -0.219781, amsgrad w = -0.177873
step 3 (grad=0.1): plain Adam w = -0.262996, amsgrad w = -0.199465
step 4 (grad=0.1): plain Adam w = -0.299795, amsgrad w = -0.215906
step 5 (grad=0.1): plain Adam w = -0.331773, amsgrad w = -0.228946
step 6 (grad=0.1): plain Adam w = -0.359953, amsgrad w = -0.239582
```

By step 6, plain `Adam` has moved noticeably farther than `amsgrad` --
its per-step increment (roughly `0.028`-`0.032` in the later steps) is
consistently larger than `amsgrad`'s (roughly `0.01`-`0.013`), exactly
because `amsgrad` never lets the spike's effect on the second moment
fully fade.

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

  optim::Adam optimizer(*model, optim::AdamOptions{.learning_rate = 0.2});

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
epoch 2: loss = 1.45141
epoch 3: loss = 0.671543
epoch 4: loss = 0.20399
```

### Example 4

`Adam` is not platform-specific -- Example 3's training loop runs
unchanged on MPS, just by constructing the model and data with
`Config{}.with_platform(pluss::pMPS)` instead of `pCPU`.
`Series::item<T>()` requires CPU-resident data, so the loss is moved back
with `.to(pluss::pCPU)` purely for printing.

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

  const Config mps_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
  nn::Fusion model(2, 1, /*bias=*/false, mps_cfg);
  const Series w = pluss::make_series({0.5F, -0.5F}, {1, 2}, mps_cfg);
  model->weight().copy_(w);

  const Series x = pluss::make_series({1.0F, 1.0F}, {1, 2}, mps_cfg);
  const Series y = pluss::make_series({2.0F}, {1, 1}, mps_cfg);

  optim::Adam optimizer(*model, optim::AdamOptions{.learning_rate = 0.2});

  for (int epoch = 0; epoch < 5; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      const Series prediction = model.forward(x);
      return nn::losses::MSE(prediction, y);
    });
    std::cout << "epoch " << epoch << " (platform=" << loss.platform()
               << "): loss = " << loss.to(pluss::pCPU).item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
epoch 0 (platform=mps): loss = 4
epoch 1 (platform=mps): loss = 2.55999
epoch 2 (platform=mps): loss = 1.45141
epoch 3 (platform=mps): loss = 0.671543
epoch 4 (platform=mps): loss = 0.203989
```

Matches Example 3's CPU trajectory up to backend floating-point rounding.

## Changelog

- **0.1** — bias-corrected first/second moment estimation, optional
  AMSGrad, weight decay, `maximize`, clipping, and lane execution control.

## See Also

- [`AdamOptions`](AdamOptions.md) -- every field's default and effect.
- [`AdamGroup`](AdamGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`AdamW`](AdamW.md) -- `Adam` with weight decay decoupled from the
  gradient.
- [`NAdam`](NAdam.md) -- `Adam` with Nesterov momentum folded into the
  first moment.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

AdamGroup
AdamOptions
```
<!-- apiref:toctree:end -->
