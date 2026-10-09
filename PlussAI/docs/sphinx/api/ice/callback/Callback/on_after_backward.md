<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cabcab3cee66 -->
# `pluss::ai::ice::Callback::on_after_backward`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_after_backward(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per training batch, right after `.reverse()` has run -- every
registered parameter now has a gradient
([`Series::has_sensitivity()`](../RunState.md)) whether or not this
batch will actually trigger an optimizer step (that depends on
`accumulation_steps`; see [`on_before_step`](on_before_step.md)). The
default implementation does nothing. This is the point to inspect or
log gradient-level information -- a gradient norm, a per-parameter
statistic -- before the optimizer ever touches the parameters. A stop
requested here still skips the optimizer step for this batch (so the
parameter values themselves are unaffected), but unlike
[`on_before_backward`](on_before_backward.md), the gradient has already
been computed by this point.

## Parameters

`RunState& state`
: `state.parameters`' entries now have a populated gradient --
  `has_sensitivity()` reads `true`.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: confirming it fires once per training batch, the
gradient becoming available exactly at this point (not before), and a
stop requested here leaving parameters unchanged even though the
gradient was already computed.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountAfterBackwardCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_after_backward(pluss::ai::ice::RunState&) override { ++fire_count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.5;
  options.shuffle = false;

  const auto counter = std::make_shared<CountAfterBackwardCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_after_backward fired " << counter->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_after_backward fired 1 time(s)
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class ShowGradientAppearsCallback final : public pluss::ai::ice::Callback {
 public:
  void on_before_backward(pluss::ai::ice::RunState& state) override {
    std::cout << "on_before_backward: has_sensitivity() = "
               << state.parameters->front()->has_sensitivity() << "\n";
  }
  void on_after_backward(pluss::ai::ice::RunState& state) override {
    std::cout << "on_after_backward:  has_sensitivity() = "
               << state.parameters->front()->has_sensitivity() << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.5;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<ShowGradientAppearsCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_before_backward: has_sensitivity() = 0
on_after_backward:  has_sensitivity() = 1
```

### Example 3

Stopping here still skips the optimizer step -- the parameter's value
is unchanged at the end of the run -- but `has_sensitivity()` confirms
the gradient itself was already computed before the stop took effect.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopAfterBackwardCallback final : public pluss::ai::ice::Callback {
 public:
  void on_after_backward(pluss::ai::ice::RunState& state) override {
    state.stop_requested = true;
  }
};

class ShowParamAndGradientCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "before batch: param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
  }
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "after run:    param[0].sum() = " << state.parameters->front()->sum().item<float>()
               << ", has_sensitivity() = " << state.parameters->front()->has_sensitivity() << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.5;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<ShowParamAndGradientCallback>());
  coach.add_callback(std::make_shared<StopAfterBackwardCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
before batch: param[0].sum() = -0.599627
after run:    param[0].sum() = -0.599627, has_sensitivity() = 1
```

## See Also

- [`on_before_backward`](on_before_backward.md) -- fires right before,
  with no gradient available yet.
- [`on_before_step`](on_before_step.md) -- the next hook, only firing
  on batches that actually trigger an optimizer step.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state.html">
    <span class="label">Previous</span>
    <span class="title">load_state</span>
  </a>
  <a class="next-link" href="on_batch_begin.html">
    <span class="label">Next</span>
    <span class="title">on_batch_begin</span>
  </a>
</div>
<!-- apiref:nav:end -->
