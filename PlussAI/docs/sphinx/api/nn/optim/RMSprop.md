# `pluss::ai::nn::optim::RMSprop`

Root-mean-square gradient scaling: each parameter keeps a decayed running
average of its own squared gradient (`square_avg = alpha * square_avg +
(1 - alpha) * gradient^2`), and the update divides the raw gradient by
that average's square root -- similar in spirit to
[`Adagrad`](Adagrad.md)'s accumulator, but a *decayed* average instead of
an ever-growing sum, so the effective learning rate can recover after a
quiet stretch rather than only ever shrinking. Optional momentum applies
on top of the scaled update, and an optional `centered` mode uses the
gradient's variance (subtracting out its own running mean) instead of its
raw second moment. Every method not specific to `RMSprop`'s own formula is
inherited unchanged from [`Optimizer`](Optimizer.md).

## Signature

```cpp
explicit nn::optim::RMSprop::RMSprop(pluss::RangeView<pluss::Series*> parameters, nn::optim::RMSpropOptions options = {});
explicit nn::optim::RMSprop::RMSprop(pluss::RangeView<nn::optim::RMSpropGroup> groups);
explicit nn::optim::RMSprop::RMSprop(nn::Module& module, nn::optim::RMSpropOptions options = {});

const nn::optim::RMSpropOptions& nn::optim::RMSprop::options() const noexcept;
void nn::optim::RMSprop::set_options(const nn::optim::RMSpropOptions& options);
void nn::optim::RMSprop::set_group_options(std::size_t group_index, const nn::optim::RMSpropOptions& options);
std::size_t nn::optim::RMSprop::group_count() const noexcept;

nn::optim::RMSprop::State nn::optim::RMSprop::capture_state() const;
void nn::optim::RMSprop::restore_state(const nn::optim::RMSprop::State& state);
```

`State` carries each parameter's squared-gradient running average, its
momentum buffer (when `momentum > 0`), and its running gradient average
(when `centered = true`), plus each group's own options.

## Construction

Same three-overload shape as [`SGD`](SGD.md): an explicit parameter list
plus shared [`RMSpropOptions`](RMSpropOptions.md), a per-group
[`RMSpropGroup`](RMSpropGroup.md) list, or a `Module&`.

## State

Same shape as [`SGD`](SGD.md)'s own `## State`.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Three examples: baseline convergence, `centered`'s effect against plain
`RMSprop`, and an end-to-end `Module` training loop.

### Example 1

A single parameter converging toward a target -- `RMSprop`'s running
average adapts quickly enough at this learning rate that it briefly
overshoots the target before settling.

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
  optim::RMSprop optimizer(std::vector<Series*>{&w}, optim::RMSpropOptions{.learning_rate = 0.5});

  for (int step = 0; step < 8; ++step) {
    optimizer.step(std::vector<Series>{w.sub(target)});
    std::cout << "step " << step << ": w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: w = -2.38419e-06
step 1: w = 1.21843
step 2: w = 0.951331
step 3: w = 1.01114
step 4: w = 0.997381
step 5: w = 1.00063
step 6: w = 0.999844
step 7: w = 1.00004
```

### Example 2

`centered = true` scales the update by the gradient's estimated
*variance* (subtracting out its own running mean) instead of its raw
second moment -- on this quadratic, where the gradient's sign is
consistent throughout (always pulling toward the target, never
oscillating), the two modes produce a close but measurably different
trajectory.

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

  Series w_plain = pluss::make_series(5.0F, f32);
  optim::RMSprop plain(std::vector<Series*>{&w_plain}, optim::RMSpropOptions{.learning_rate = 0.1});

  Series w_centered = pluss::make_series(5.0F, f32);
  optim::RMSprop centered(std::vector<Series*>{&w_centered}, optim::RMSpropOptions{.learning_rate = 0.1, .centered = true});

  for (int step = 0; step < 6; ++step) {
    plain.step(std::vector<Series>{w_plain.sub(target)});
    centered.step(std::vector<Series>{w_centered.sub(target)});
    std::cout << "step " << step << ": centered=false w = " << w_plain.item<float>()
               << ", centered=true w = " << w_centered.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: centered=false w = 4, centered=true w = 3.99496
step 1: centered=false w = 3.39807, centered=true w = 3.38773
step 2: centered=false w = 2.96272, centered=true w = 2.94739
step 3: centered=false w = 2.62558, centered=true w = 2.60587
step 4: centered=false w = 2.35538, centered=true w = 2.33207
step 5: centered=false w = 2.13454, centered=true w = 2.10849
```

### Example 3

An end-to-end training loop via the closure `step` overload, with a
gentler learning rate than Examples 1-2 to show gradual convergence rather
than reaching the minimum in a single call.

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

  optim::RMSprop optimizer(*model, optim::RMSpropOptions{.learning_rate = 0.02});

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
epoch 1: loss = 2.56
epoch 2: loss = 1.82076
epoch 3: loss = 1.34962
epoch 4: loss = 1.02273
```

## Changelog

- **0.1** — the decayed-running-average adaptive optimizer, with optional
  momentum, centering, weight decay, `maximize`, clipping, and lane
  execution control.

## See Also

- [`RMSpropOptions`](RMSpropOptions.md) -- every field's default and
  effect.
- [`RMSpropGroup`](RMSpropGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`Adagrad`](Adagrad.md) -- the ever-growing-accumulator predecessor
  `RMSprop`'s decayed average was designed to improve on.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

RMSpropGroup
RMSpropOptions
```
<!-- apiref:toctree:end -->
