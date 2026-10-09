# `pluss::ai::nn::optim::SGD`

Stochastic gradient descent: the simplest update rule in this reference,
`param -= learning_rate * gradient`, optionally smoothed by a momentum
buffer and dampening, and optionally applying weight decay and/or
Nesterov's lookahead correction. Every method not specific to SGD's own
formula (`step`, `parameters`, `size`, `clear`,
`last_step_lane_telemetry`) is inherited unchanged from
[`Optimizer`](Optimizer.md) -- this page covers only what `SGD` itself
adds: its own `Options`, its own per-parameter velocity state, and the
momentum/Nesterov/weight-decay formula.

## Signature

```cpp
explicit nn::optim::SGD::SGD(pluss::RangeView<pluss::Series*> parameters, nn::optim::SGDOptions options = {});
explicit nn::optim::SGD::SGD(pluss::RangeView<nn::optim::SGDGroup> groups);
explicit nn::optim::SGD::SGD(nn::Module& module, nn::optim::SGDOptions options = {});

const nn::optim::SGDOptions& nn::optim::SGD::options() const noexcept;
void nn::optim::SGD::set_options(const nn::optim::SGDOptions& options);
void nn::optim::SGD::set_group_options(std::size_t group_index, const nn::optim::SGDOptions& options);
std::size_t nn::optim::SGD::group_count() const noexcept;

nn::optim::SGD::State nn::optim::SGD::capture_state() const;
void nn::optim::SGD::restore_state(const nn::optim::SGD::State& state);
```

`State` is a plain aggregate (`group_options`, `velocity`) -- see `##
State` below.

## Construction

```cpp
// a single group, shared options
nn::optim::SGD a(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1, .momentum = 0.9});

// per-group options -- see SGDGroup
std::vector<nn::optim::SGDGroup> groups{
    {{&backbone_weight}, nn::optim::SGDOptions{.learning_rate = 0.01}},
    {{&head_weight}, nn::optim::SGDOptions{.learning_rate = 0.1}},
};
nn::optim::SGD b(groups);

// from a Module directly
nn::optim::SGD c(*model, nn::optim::SGDOptions{.learning_rate = 0.1});
```

Every constructor forwards its parameter list to
[`Optimizer`](Optimizer.md)'s own construction (validation, forced
`tracks_slope_(true)`, ...), then allocates one zero-initialized velocity
buffer per parameter.

## State

`options()` / `set_options(options)`
: Read or replace the default [`SGDOptions`](SGDOptions.md) -- for a
  single-group `SGD`, this is the only group's options; for a per-group
  `SGD`, `set_options` overwrites every group's options with the same
  value, while `set_group_options` targets one group at a time.

`group_count()`
: How many parameter groups this optimizer has -- `1` unless constructed
  from a `Group` list.

`capture_state()` / `restore_state(state)`
: Captures/restores SGD's own internal state -- each group's `SGDOptions`
  plus the velocity buffer for every parameter -- as a plain `State`
  value. This is independent of parameter *values*: restoring a captured
  state resumes momentum exactly where it left off, but never moves a
  parameter itself (see Example 4). Pair with
  [`Module::snapshot`](../module/Module/snapshot.md)/
  [`restore`](../module/Module/restore.md) to check-point both halves of
  a training run.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Five examples: the effect of momentum against plain SGD, Nesterov's
lookahead correction against classic momentum, an end-to-end `Module`
training loop via the closure `step` overload, a `capture_state`/
`restore_state` round trip, and the same end-to-end loop moved onto MPS.

### Example 1

Two `SGD` instances minimize the same quadratic from the same starting
point -- one plain, one with `momentum = 0.9`. Momentum accelerates
descent by carrying a fraction of the previous update forward; by step 5
it has overshot past the target (`1.0`) while plain SGD is still
approaching it monotonically.

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
  optim::SGD plain(std::vector<Series*>{&w_plain}, optim::SGDOptions{.learning_rate = 0.1});

  Series w_momentum = pluss::make_series(5.0F, f32);
  optim::SGD momentum(std::vector<Series*>{&w_momentum}, optim::SGDOptions{.learning_rate = 0.1, .momentum = 0.9});

  for (int step = 0; step < 6; ++step) {
    optim::SGD* opts[2] = {&plain, &momentum};
    Series* ws[2] = {&w_plain, &w_momentum};
    for (int i = 0; i < 2; ++i) {
      const Series gradient = ws[i]->sub(target);
      opts[i]->step(std::vector<Series>{gradient});
    }
    std::cout << "step " << step << ": plain w = " << w_plain.item<float>()
               << ", momentum w = " << w_momentum.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: plain w = 4.6, momentum w = 4.6
step 1: plain w = 4.24, momentum w = 3.88
step 2: plain w = 3.916, momentum w = 2.944
step 3: plain w = 3.6244, momentum w = 1.9072
step 4: plain w = 3.36196, momentum w = 0.88336
step 5: plain w = 3.12576, momentum w = -0.0264322
```

### Example 2

`nesterov = true` evaluates the momentum-adjusted gradient one step ahead
before applying it, rather than applying plain momentum and correcting
afterward -- both use the same `momentum = 0.9`, but Nesterov reaches
lower values sooner on this quadratic.

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

  Series w_classic = pluss::make_series(5.0F, f32);
  optim::SGD classic(std::vector<Series*>{&w_classic},
                     optim::SGDOptions{.learning_rate = 0.1, .momentum = 0.9, .nesterov = false});

  Series w_nesterov = pluss::make_series(5.0F, f32);
  optim::SGD nesterov(std::vector<Series*>{&w_nesterov},
                      optim::SGDOptions{.learning_rate = 0.1, .momentum = 0.9, .nesterov = true});

  for (int step = 0; step < 6; ++step) {
    const Series g_classic = w_classic.sub(target);
    const float classic_loss = g_classic.square().mul(0.5).item<float>();
    classic.step(std::vector<Series>{g_classic});
    const Series g_nesterov = w_nesterov.sub(target);
    const float nesterov_loss = g_nesterov.square().mul(0.5).item<float>();
    nesterov.step(std::vector<Series>{g_nesterov});
    std::cout << "step " << step << ": classic loss = " << classic_loss << ", nesterov loss = " << nesterov_loss
              << ", classic momentum w = " << w_classic.item<float>()
               << ", nesterov w = " << w_nesterov.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: classic loss = 8, nesterov loss = 8, classic momentum w = 4.6, nesterov w = 4.24
step 1: classic loss = 6.48, nesterov loss = 5.2488, classic momentum w = 3.88, nesterov w = 3.3004
step 2: classic loss = 4.1472, nesterov loss = 2.64592, classic momentum w = 2.944, nesterov w = 2.30928
step 3: classic loss = 1.88957, nesterov loss = 0.857112, classic momentum w = 1.9072, nesterov w = 1.37555
step 4: classic loss = 0.411506, nesterov loss = 0.0705194, classic momentum w = 0.88336, nesterov w = 0.581673
step 5: classic loss = 0.00680247, nesterov loss = 0.0874987, classic momentum w = -0.0264322, nesterov w = -0.0195355
```

`nesterov` requires `momentum > 0` -- construction throws otherwise.

### Example 3

An end-to-end training loop: a `Fusion` layer's weight is initialized by
hand, then trained against one example with the closure `step` overload
-- `step` runs the forward pass, computes MSE, backpropagates, and applies
the update, all in one call, with the loss returned for logging.

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

  optim::SGD optimizer(*model, optim::SGDOptions{.learning_rate = 0.1});

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
epoch 1: loss = 1.44
epoch 2: loss = 0.5184
epoch 3: loss = 0.186624
epoch 4: loss = 0.0671846
```

### Example 4

`capture_state()` snapshots SGD's own velocity buffer (and options) -- not
the parameter's value. Restoring an earlier state resumes momentum from
that point, but the parameter itself stays wherever training already left
it; restoring is not the same as rewinding.

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
  optim::SGD optimizer(std::vector<Series*>{&w}, optim::SGDOptions{.learning_rate = 0.1, .momentum = 0.9});

  for (int step = 0; step < 3; ++step) {
    const Series residual = w.sub(target);
    const float loss = residual.square().mul(0.5).item<float>();
    optimizer.step(std::vector<Series>{residual});
    std::cout << "step " << step << ": loss = " << loss << ", w = " << w.item<float>() << "\n";
  }
  std::cout << "after 3 steps: w = " << w.item<float>() << "\n";

  const optim::SGD::State saved = optimizer.capture_state();

  for (int step = 0; step < 3; ++step) {
    const Series residual = w.sub(target);
    const float loss = residual.square().mul(0.5).item<float>();
    optimizer.step(std::vector<Series>{residual});
    std::cout << "continued step " << step << ": loss = " << loss << ", w = " << w.item<float>() << "\n";
  }
  std::cout << "after 3 more steps: w = " << w.item<float>() << "\n";

  optimizer.restore_state(saved);
  std::cout << "after restore_state(): w itself is unchanged by restore, still = " << w.item<float>() << "\n";

  const Series residual = w.sub(target);
  const float loss = residual.square().mul(0.5).item<float>();
  optimizer.step(std::vector<Series>{residual});
  std::cout << "one more step from the restored velocity: loss = " << loss << ", w = " << w.item<float>() << "\n";

  return 0;
}
```

Output

```text
step 0: loss = 8, w = 4.6
step 1: loss = 6.48, w = 3.88
step 2: loss = 4.1472, w = 2.944
after 3 steps: w = 2.944
continued step 0: loss = 1.88957, w = 1.9072
continued step 1: loss = 0.411506, w = 0.88336
continued step 2: loss = 0.00680247, w = -0.0264322
after 3 more steps: w = -0.0264322
after restore_state(): w itself is unchanged by restore, still = -0.0264322
one more step from the restored velocity: loss = 0.526781, w = -0.766189
```

The last line's update is driven by the *restored* (step-3) velocity, not
the velocity that had actually accumulated by step 6 -- `w`'s own value,
however, was never touched by `restore_state` at all.

### Example 5

Nothing about `SGD` itself is platform-specific -- moving Example 3's
training loop onto MPS is just a matter of constructing the model and
data with `Config{}.with_platform(pluss::pMPS)` instead of `pCPU`; `SGD`
runs unchanged. `Series::item<T>()` requires CPU-resident data, so the
loss is moved back with `.to(pluss::pCPU)` purely for printing.

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

  optim::SGD optimizer(*model, optim::SGDOptions{.learning_rate = 0.1});

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
epoch 1 (platform=mps): loss = 1.44
epoch 2 (platform=mps): loss = 0.5184
epoch 3 (platform=mps): loss = 0.186624
epoch 4 (platform=mps): loss = 0.0671846
```

Identical trajectory to Example 3's CPU run -- moving platforms changes
where the computation runs, never the math.

## Changelog

- **0.1** — plain SGD with momentum, dampening, weight decay, Nesterov,
  `maximize`, clipping, and lane execution control.

## See Also

- [`SGDOptions`](SGDOptions.md) -- every field's default and effect.
- [`SGDGroup`](SGDGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base every method not listed
  under `## Methods` above comes from.
- [`Adam`](Adam.md) -- an adaptive alternative when a single global
  learning rate across all parameters is not working well.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SGDGroup
SGDOptions
```
<!-- apiref:toctree:end -->
