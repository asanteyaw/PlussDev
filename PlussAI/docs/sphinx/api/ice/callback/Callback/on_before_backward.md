<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ff434f2095dd -->
# `pluss::ai::ice::Callback::on_before_backward`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_before_backward(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per training batch, right after that batch's loss has been
computed and written to `state.metrics["loss"]`, but before
`.reverse()` ever runs. The default implementation does nothing. Only
fires for `Learn`-stage batches during `Coach::learn` -- never for
validation, test, or inference batches, since none of those compute a
gradient. This is the earliest point to inspect a batch's loss before
any compute is spent on backward/step; a stop requested here skips the
backward pass and the optimizer step entirely for this batch -- nothing
about it is partially applied.

## Parameters

`RunState& state`
: `state.metric("loss")` is this batch's loss value.
  `state.parameters` still reflects the batch's *input* values --
  unchanged from [`on_batch_begin`](on_batch_begin.md), since neither
  backward nor the optimizer step has run yet.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: confirming it only fires for `Learn`-stage batches,
confirming parameters are still unchanged at this point, and a stop
requested here skipping backward/step entirely, leaving parameters
untouched for the whole run.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <string>

namespace {

std::string stage_name(pluss::ai::ice::StepStage stage) {
  using pluss::ai::ice::StepStage;
  switch (stage) {
    case StepStage::Learn: return "Learn";
    case StepStage::Valid: return "Valid";
    case StepStage::Test: return "Test";
    case StepStage::Infer: return "Infer";
  }
  return "?";
}

class LogBeforeBackwardCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_before_backward(pluss::ai::ice::RunState& state) override {
    ++fire_count;
    std::cout << "on_before_backward: stage = " << stage_name(state.stage)
               << ", loss = " << state.metric("loss").value_or(-1.0) << "\n";
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
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.5;
  options.shuffle = false;

  const auto logger = std::make_shared<LogBeforeBackwardCallback>();
  ice::Coach coach(options);
  coach.add_callback(logger);
  coach.learn(model, x, y);

  // Only the Learn-stage batch triggers this hook -- the Valid-stage
  // batch in the same epoch does not.
  std::cout << "on_before_backward fired " << logger->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_before_backward: stage = Learn, loss = 1.2158
on_before_backward fired 1 time(s)
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class ShowParamUnchangedCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_begin:      param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
  }
  void on_before_backward(pluss::ai::ice::RunState& state) override {
    std::cout << "on_before_backward:  param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
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
  coach.add_callback(std::make_shared<ShowParamUnchangedCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_batch_begin:      param[0].sum() = -0.599627
on_before_backward:  param[0].sum() = -0.599627
```

### Example 3

Requesting a stop from `on_before_backward` skips backward and the
optimizer step entirely for that batch -- the parameter value at the
end of the whole run is identical to what it was before the first
batch ever started.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopBeforeBackwardCallback final : public pluss::ai::ice::Callback {
 public:
  void on_before_backward(pluss::ai::ice::RunState& state) override {
    state.stop_requested = true;
  }
};

class ShowParamCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "before batch: param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
  }
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "after run:    param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
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
  coach.add_callback(std::make_shared<ShowParamCallback>());
  coach.add_callback(std::make_shared<StopBeforeBackwardCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
before batch: param[0].sum() = -0.599627
after run:    param[0].sum() = -0.599627
```

## See Also

- [`on_after_backward`](on_after_backward.md) -- fires right after,
  once `.reverse()` has run.
- [`on_before_step`](on_before_step.md) -- the last hook before the
  optimizer step itself.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_batch_end.html">
    <span class="label">Previous</span>
    <span class="title">on_batch_end</span>
  </a>
  <a class="next-link" href="on_before_step.html">
    <span class="label">Next</span>
    <span class="title">on_before_step</span>
  </a>
</div>
<!-- apiref:nav:end -->
