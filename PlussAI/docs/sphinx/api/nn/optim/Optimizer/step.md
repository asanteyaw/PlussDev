<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b8a87e9c11d -->

# `pluss::ai::nn::optim::Optimizer::step`

[Back to `pluss::ai::nn::optim::Optimizer`](../Optimizer.md)

## Signature

```cpp
void nn::optim::Optimizer::step();
void nn::optim::Optimizer::step(pluss::RangeView<pluss::Series> gradients);

template <typename Closure>
requires(std::invocable<Closure&> &&
         std::same_as<std::invoke_result_t<Closure&>, pluss::Series>)
pluss::Series nn::optim::Optimizer::step(Closure&& closure);

template <typename Closure>
requires(std::invocable<Closure&> &&
         std::same_as<std::invoke_result_t<Closure&>,
                      std::pair<pluss::Series, std::vector<pluss::Series>>>)
pluss::Series nn::optim::Optimizer::step(Closure&& closure);
```

Four overloads, distinguished by how the gradient each parameter needs
this step is supplied. Every concrete optimizer inherits all four
unchanged; only the update formula each one runs after collecting the
gradient differs.

## What It Does

Every overload ends the same way: for each parameter group, the concrete
optimizer's own update formula runs (using whatever hyperparameters that
group's `Options` carries), and every parameter is reassigned to its next
value as a fresh leaf -- gradient tracking is preserved, but the parameter's
own recorded sensitivity is cleared as part of that reassignment. What
differs between the four overloads is *where the gradient itself comes
from*:

- **No arguments** -- reads each parameter's own already-recorded
  sensitivity (`Series::sensitivity()`), exactly as an earlier
  `some_loss.reverse()` call left it. This is the manual, lowest-level
  form: you built the graph, called `.reverse()` yourself, and now hand
  control to the optimizer for just the update step.
- **`gradients`** -- takes one `Series` per parameter, in parameter order,
  supplied by the caller directly. No autodiff runs at all; this is the
  overload to reach for when a gradient is already known some other way
  (computed analytically, or produced by machinery outside this engine's
  own tape).
- **Closure returning `Series`** -- the closure computes and returns a
  loss from tracked ops on the parameters; `step` itself calls
  [`clear`](clear.md), invokes the closure, calls `.reverse()` on the
  result, then runs the no-arg `step()`. This is full autodiff end to end,
  and the form most training loops actually use -- see the [`optim`
  landing page](../../optim.md)'s own example.
- **Closure returning `pair<Series, vector<Series>>`** -- the closure
  returns both a loss (for logging only -- never `.reverse()`d) and the
  gradients it already computed by hand; `step` forwards those gradients
  straight to the `gradients` overload. Reach for this when gradients are
  cheap to compute analytically but a loss value for logging is still
  wanted from the same call.

## Parameters

`pluss::RangeView<pluss::Series> gradients`
: One gradient per parameter, in the same order as
  [`parameters()`](parameters.md). Size must match
  [`size()`](size.md) exactly -- mismatched size throws. An individual
  entry may be an undefined `Series` (`pluss::series::undefined()`),
  meaning "no update for this parameter this step"; every other entry must
  share that parameter's platform, dtype, and shape (a `numel() == 1`
  scalar broadcasts against any shape).

`Closure&& closure`
: A callable taking no arguments. Which of the two closure overloads
  resolves depends entirely on its return type -- return a plain `Series`
  for the full-autodiff form, or a `std::pair<Series, vector<Series>>` for
  the explicit-gradient-with-loss form. The loss (either return shape) must
  be a real (non-complex) floating dtype; a non-finite loss throws when
  [`StepDiagnosticsOptions`](../StepDiagnosticsOptions.md)`.fail_if_non_finite_loss`
  is set.

## Returns

`void` for the no-arg and `gradients` overloads -- there is no loss value
to hand back since neither one computed one. `Series` for both closure
overloads -- the loss the closure returned, unmodified, for logging.

## Errors

- `gradients.size() != size()` throws.
- A closure returning an undefined loss throws.
- A closure's loss with a non-floating, non-real dtype throws (checked via
  the same training-dtype gate every loss function in this reference
  uses).
- Every [`StepDiagnosticsOptions`](../StepDiagnosticsOptions.md) check that
  is turned on can throw from any overload it applies to -- see that
  page for exactly which overloads each check covers.
- A gradient/parameter platform, dtype, or shape mismatch throws (from any
  overload that supplies explicit gradients).

## Practical Examples

Four examples, one per overload -- all minimizing the same tiny quadratic
`f(w) = 0.5 * (w - target)^2` (analytic gradient `w - target`) via plain
[`SGD`](../SGD.md), so the only thing that changes between them is how the
gradient reaches `step`.

### Example 1

The no-arg overload: track `w`'s slope, compute the loss with ordinary
tracked ops, call `.reverse()` by hand, then let `step()` read the
resulting sensitivity.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  w.tracks_slope_(true);
  const Series target = pluss::make_series(1.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1});

  for (int step = 0; step < 5; ++step) {
    optimizer.clear();
    const Series loss = w.sub(target).square().mul(0.5);
    loss.reverse();
    optimizer.step();
    std::cout << "step " << step << ": loss = " << loss.item<float>() << ", w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: loss = 8, w = 4.6
step 1: loss = 6.48, w = 4.24
step 2: loss = 5.2488, w = 3.916
step 3: loss = 4.25153, w = 3.6244
step 4: loss = 3.44374, w = 3.36196
```

### Example 2

The `gradients` overload: the same quadratic's gradient is known in closed
form (`w - target`), computed by hand, and handed to `step` directly --
`w` never needs `tracks_slope_(true)` at all, since no autodiff runs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  const Series target = pluss::make_series(1.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1});

  for (int step = 0; step < 5; ++step) {
    const Series gradient = w.sub(target);
    optimizer.step(std::vector<Series>{gradient});
    std::cout << "step " << step << ": gradient = " << gradient.item<float>() << ", w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: gradient = 4, w = 4.6
step 1: gradient = 3.6, w = 4.24
step 2: gradient = 3.24, w = 3.916
step 3: gradient = 2.916, w = 3.6244
step 4: gradient = 2.6244, w = 3.36196
```

Identical trajectory to Example 1 -- same math, reached without building or
reversing a graph at all.

### Example 3

The closure-returning-`Series` overload: the closure computes the loss
from tracked ops, and `step` handles `clear()`/`.reverse()`/the no-arg
`step()` internally -- Example 1's four lines collapse into one call, and
the returned loss is available for logging without a separate variable.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  w.tracks_slope_(true);
  const Series target = pluss::make_series(1.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1});

  for (int step = 0; step < 5; ++step) {
    const Series loss = optimizer.step([&]() -> Series {
      return w.sub(target).square().mul(0.5);
    });
    std::cout << "step " << step << ": loss = " << loss.item<float>() << ", w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: loss = 8, w = 4.6
step 1: loss = 6.48, w = 4.24
step 2: loss = 5.2488, w = 3.916
step 3: loss = 4.25153, w = 3.6244
step 4: loss = 3.44374, w = 3.36196
```

### Example 4

The closure-returning-`pair<Series, vector<Series>>` overload: the loss
and the analytic gradient from Example 2 are computed together and
returned as a pair -- `step` forwards the gradient to the same path
Example 2 used, but the caller still gets a loss back for logging, in one
call, with no autodiff at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <utility>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  const Series target = pluss::make_series(1.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1});

  for (int step = 0; step < 5; ++step) {
    const Series loss = optimizer.step([&]() -> std::pair<Series, std::vector<Series>> {
      const Series diff = w.sub(target);
      return {diff.square().mul(0.5), std::vector<Series>{diff}};
    });
    std::cout << "step " << step << ": loss = " << loss.item<float>() << ", w = " << w.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
step 0: loss = 8, w = 4.6
step 1: loss = 6.48, w = 4.24
step 2: loss = 5.2488, w = 3.916
step 3: loss = 4.25153, w = 3.6244
step 4: loss = 3.44374, w = 3.36196
```

## See Also

- [`clear`](clear.md) -- what the no-arg overload's manual pattern needs
  called between rounds when a parameter can be disconnected from one
  round's loss but not the next; also what every overload runs internally
  as part of applying its own update.
- [`GradientClipOptions`](../GradientClipOptions.md) /
  [`ExecutionOptions`](../ExecutionOptions.md) -- every concrete
  optimizer's own `Options` (here, [`SGDOptions`](../SGDOptions.md)) ends
  with these two, applied identically regardless of which `step` overload
  supplied the gradient.
- [`last_step_lane_telemetry`](last_step_lane_telemetry.md) -- inspect
  which update lane the most recent `step()` call actually ran.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
