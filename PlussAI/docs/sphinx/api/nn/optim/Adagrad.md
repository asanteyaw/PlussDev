# `pluss::ai::nn::optim::Adagrad`

Adaptive gradient descent: each parameter accumulates the running sum of
its own squared gradients, and the effective learning rate for that
parameter shrinks as its own accumulator grows --
`accumulator += gradient^2; update = gradient / (sqrt(accumulator) +
epsilon)`. Parameters that have consistently received large gradients get
a smaller effective step; parameters that have barely moved keep a larger
one. Every method not specific to `Adagrad`'s own formula is inherited
unchanged from [`Optimizer`](Optimizer.md) -- see that page for `step`,
`parameters`, `size`, `clear`, `last_step_lane_telemetry`.

## Signature

```cpp
explicit nn::optim::Adagrad::Adagrad(pluss::RangeView<pluss::Series*> parameters, nn::optim::AdagradOptions options = {});
explicit nn::optim::Adagrad::Adagrad(pluss::RangeView<nn::optim::AdagradGroup> groups);
explicit nn::optim::Adagrad::Adagrad(nn::Module& module, nn::optim::AdagradOptions options = {});

const nn::optim::AdagradOptions& nn::optim::Adagrad::options() const noexcept;
void nn::optim::Adagrad::set_options(const nn::optim::AdagradOptions& options);
void nn::optim::Adagrad::set_group_options(std::size_t group_index, const nn::optim::AdagradOptions& options);
std::size_t nn::optim::Adagrad::group_count() const noexcept;

nn::optim::Adagrad::State nn::optim::Adagrad::capture_state() const;
void nn::optim::Adagrad::restore_state(const nn::optim::Adagrad::State& state);
```

`State` is a plain aggregate (`group_options`, `sum_square`) -- the
accumulated-squared-gradient buffer for every parameter, plus each group's
own options.

## Construction

Same three-overload shape as [`SGD`](SGD.md): an explicit parameter list
plus shared [`AdagradOptions`](AdagradOptions.md), a per-group
[`AdagradGroup`](AdagradGroup.md) list, or a `Module&`.

```cpp
nn::optim::Adagrad a(std::vector<Series*>{&w}, nn::optim::AdagradOptions{.learning_rate = 0.1});
nn::optim::Adagrad b(*model, nn::optim::AdagradOptions{.learning_rate = 0.1});
```

## State

Same shape as [`SGD`](SGD.md)'s own `## State`: `options()`/
`set_options()`/`set_group_options()`/`group_count()` read or replace
[`AdagradOptions`](AdagradOptions.md), and `capture_state()`/
`restore_state()` snapshot Adagrad's own internal state -- here, the
per-parameter squared-gradient accumulator -- independent of the
parameter's own value.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Three examples: the adaptive scaling Adagrad's accumulator produces
against plain SGD on parameters with very different gradient magnitudes,
`lr_decay`'s effect over several steps, and an end-to-end `Module`
training loop.

### Example 1

Two parameters, both driven by a *constant* gradient every step -- one
small (`0.1`), one 100x larger (`10.0`). Plain SGD moves the large-gradient
parameter 100x farther than the small one, every step. Adagrad's
accumulator scales each parameter's own update by its own gradient
history, so after a few steps both parameters have moved by almost
exactly the same amount -- the accumulator exactly cancels a constant
gradient's own magnitude.

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

  Series small_grad_sgd = pluss::make_series(0.0F, f32);
  Series big_grad_sgd = pluss::make_series(0.0F, f32);
  optim::SGD sgd(std::vector<Series*>{&small_grad_sgd, &big_grad_sgd}, optim::SGDOptions{.learning_rate = 0.1});

  Series small_grad_ada = pluss::make_series(0.0F, f32);
  Series big_grad_ada = pluss::make_series(0.0F, f32);
  optim::Adagrad adagrad(std::vector<Series*>{&small_grad_ada, &big_grad_ada}, optim::AdagradOptions{.learning_rate = 0.1});

  for (int step = 0; step < 5; ++step) {
    sgd.step(std::vector<Series>{pluss::make_series(0.1F, f32), pluss::make_series(10.0F, f32)});
    adagrad.step(std::vector<Series>{pluss::make_series(0.1F, f32), pluss::make_series(10.0F, f32)});
  }

  std::cout << "SGD:     small moved " << -small_grad_sgd.item<float>() << ", big moved " << -big_grad_sgd.item<float>()
             << " (ratio = " << (big_grad_sgd.item<float>() / small_grad_sgd.item<float>()) << ")\n";
  std::cout << "Adagrad: small moved " << -small_grad_ada.item<float>() << ", big moved " << -big_grad_ada.item<float>()
             << " (ratio = " << (big_grad_ada.item<float>() / small_grad_ada.item<float>()) << ")\n";

  return 0;
}
```

Output

```text
SGD:     small moved 0.05, big moved 5 (ratio = 100)
Adagrad: small moved 0.323167, big moved 0.323167 (ratio = 1)
```

### Example 2

`lr_decay > 0` shrinks the effective learning rate over time
(`learning_rate / (1 + (step - 1) * lr_decay)`), independent of the
accumulator -- both optimizers below start identically at step 0, but the
decayed one's steps shrink further with every call.

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
  optim::Adagrad plain(std::vector<Series*>{&w_plain}, optim::AdagradOptions{.learning_rate = 0.5});

  Series w_decay = pluss::make_series(5.0F, f32);
  optim::Adagrad decayed(std::vector<Series*>{&w_decay}, optim::AdagradOptions{.learning_rate = 0.5, .lr_decay = 0.5});

  for (int step = 0; step < 6; ++step) {
    plain.step(std::vector<Series>{w_plain.sub(target)});
    decayed.step(std::vector<Series>{w_decay.sub(target)});
    std::cout << "step " << step << ": lr_decay=0 -> w = " << w_plain.item<float>()
               << ", lr_decay=0.5 -> w = " << w_decay.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: lr_decay=0 -> w = 4.5, lr_decay=0.5 -> w = 4.5
step 1: lr_decay=0 -> w = 4.17075, lr_decay=0.5 -> w = 4.2805
step 2: lr_decay=0 -> w = 3.91459, lr_decay=0.5 -> w = 4.14919
step 3: lr_decay=0 -> w = 3.70156, lr_decay=0.5 -> w = 4.05915
step 4: lr_decay=0 -> w = 3.51791, lr_decay=0.5 -> w = 3.99237
step 5: lr_decay=0 -> w = 3.35597, lr_decay=0.5 -> w = 3.94024
```

### Example 3

An end-to-end training loop via the closure `step` overload -- identical
shape to [`SGD`](SGD.md)'s own Example 3, with `Adagrad` in place of
`SGD`.

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

  optim::Adagrad optimizer(*model, optim::AdagradOptions{.learning_rate = 0.5});

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
epoch 1: loss = 1
epoch 2: loss = 0.305573
epoch 3: loss = 0.0978423
epoch 4: loss = 0.0317671
```

## Changelog

- **0.1** — the accumulated-squared-gradient adaptive optimizer, with
  learning-rate decay, weight decay, `maximize`, clipping, and lane
  execution control.

## See Also

- [`AdagradOptions`](AdagradOptions.md) -- every field's default and
  effect.
- [`AdagradGroup`](AdagradGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`RMSprop`](RMSprop.md) -- addresses Adagrad's own well-known weakness
  (the accumulator only ever grows, so the effective learning rate only
  ever shrinks, eventually to near zero) by using a decayed running
  average instead of a running sum.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

AdagradGroup
AdagradOptions
```
<!-- apiref:toctree:end -->
