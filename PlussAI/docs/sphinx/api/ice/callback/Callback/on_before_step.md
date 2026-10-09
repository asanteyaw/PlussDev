<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b8b18a4cb363 -->
# `pluss::ai::ice::Callback::on_before_step`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_before_step(pluss::ai::ice::RunState& state);
```

## What It Does

Fires right before the optimizer actually steps -- but unlike
[`on_before_backward`](on_before_backward.md)/
[`on_after_backward`](on_after_backward.md), **not** on every training
batch: only on a batch that actually triggers a step, per
`ExecutionPolicy::accumulation_steps` (every `accumulation_steps`
batches, plus always on the last batch of an epoch). The default
implementation does nothing. This is the point for gradient
manipulation right before it is applied -- a custom clip, a logged
gradient norm specifically at step time rather than every batch. A stop
requested here skips the step itself *and*
[`on_batch_end`](on_batch_end.md) for this batch -- the gradient stays
computed (from `on_after_backward`), but nothing consumes it.

## Parameters

`RunState& state`
: Same `state` as the rest of the batch -- `state.batch_index` is still
  this batch's index, useful for confirming which batches actually
  reach this hook under a given `accumulation_steps`.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: firing on every batch under the default
`accumulation_steps = 1`, firing only on every other batch under
`accumulation_steps = 2`, and a stop requested here skipping both the
step and that batch's `on_batch_end`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountStepHooksCallback final : public pluss::ai::ice::Callback {
 public:
  int before_step_count = 0;
  int after_backward_count = 0;
  void on_before_step(pluss::ai::ice::RunState&) override { ++before_step_count; }
  void on_after_backward(pluss::ai::ice::RunState&) override { ++after_backward_count; }
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
  options.validation.split_ratio = 0.0;
  // accumulation_steps defaults to 1 -- every batch that computes a
  // gradient also steps.

  const auto counter = std::make_shared<CountStepHooksCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_after_backward fired " << counter->after_backward_count << " time(s)\n";
  std::cout << "on_before_step fired    " << counter->before_step_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_after_backward fired 1 time(s)
on_before_step fired    1 time(s)
```

### Example 2

With `batch_size = 1`, this epoch has four batches; with
`accumulation_steps = 2`, a step (and this hook) only happens every
second batch -- `on_after_backward` still fires on all four.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountStepHooksCallback final : public pluss::ai::ice::Callback {
 public:
  int before_step_count = 0;
  int after_backward_count = 0;
  void on_before_step(pluss::ai::ice::RunState& state) override {
    ++before_step_count;
    std::cout << "on_before_step fires at batch_index = " << state.batch_index << "\n";
  }
  void on_after_backward(pluss::ai::ice::RunState&) override { ++after_backward_count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.batch_size = 1;  // one row per batch -- four batches this epoch
  options.execution.max_epochs = 1;
  options.execution.accumulation_steps = 2;  // step only every 2nd batch
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.shuffle = false;

  const auto counter = std::make_shared<CountStepHooksCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_after_backward fired " << counter->after_backward_count << " time(s)\n";
  std::cout << "on_before_step fired    " << counter->before_step_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_before_step fires at batch_index = 1
on_before_step fires at batch_index = 3
on_after_backward fired 4 time(s)
on_before_step fired    2 time(s)
```

### Example 3

A stop requested here skips the optimizer step for this batch -- the
parameter is unchanged at the end of the run -- and also skips
[`on_batch_end`](on_batch_end.md) for the same batch, confirmed by a
zero count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopBeforeStepCallback final : public pluss::ai::ice::Callback {
 public:
  void on_before_step(pluss::ai::ice::RunState& state) override {
    state.stop_requested = true;
  }
};

class ShowParamAndBatchEndCallback final : public pluss::ai::ice::Callback {
 public:
  int batch_end_count = 0;
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "before batch: param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
  }
  void on_batch_end(pluss::ai::ice::RunState&) override { ++batch_end_count; }
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "after run:    param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
    std::cout << "on_batch_end fired " << batch_end_count << " time(s)\n";
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
  coach.add_callback(std::make_shared<ShowParamAndBatchEndCallback>());
  coach.add_callback(std::make_shared<StopBeforeStepCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
before batch: param[0].sum() = -0.599627
after run:    param[0].sum() = -0.599627
on_batch_end fired 0 time(s)
```

## See Also

- [`on_after_backward`](on_after_backward.md) -- fires on every
  training batch, unlike this hook.
- [`ExecutionPolicy::accumulation_steps`](../../options/ExecutionPolicy.md) --
  what gates which batches reach this hook.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_before_backward.html">
    <span class="label">Previous</span>
    <span class="title">on_before_backward</span>
  </a>
  <a class="next-link" href="on_checkpoint_load.html">
    <span class="label">Next</span>
    <span class="title">on_checkpoint_load</span>
  </a>
</div>
<!-- apiref:nav:end -->
