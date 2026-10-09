<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: efb5cd3678e3 -->
# `pluss::ai::ice::Callback::on_batch_end`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_batch_end(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per batch, right after that batch has fully run -- the
model's forward/loss for a `Valid`/`Test`/`Infer` batch, or forward,
loss, backward, *and* (if this batch triggers one, per
`accumulation_steps`) the optimizer step for a `Learn` batch. The
default implementation does nothing. Because any optimizer step already
happened by this point, a stop requested here keeps that step's
parameter update -- unlike a stop requested from
[`on_batch_begin`](on_batch_begin.md), which cuts off before the step
ever runs.

## Parameters

`RunState& state`
: `state.metrics` holds `"loss"` (for a `Learn` batch) or `"val_loss"`
  (for a `Valid` batch), set just before this fires.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: the metric available for each stage, the parameter
update already visible by this point, and the batch-loop cutoff a stop
requested here produces.

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

class LogBatchEndCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_end(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_end: stage = " << stage_name(state.stage)
               << ", loss = " << state.metric("loss").value_or(-1.0)
               << ", val_loss = " << state.metric("val_loss").value_or(-1.0) << "\n";
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

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogBatchEndCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_batch_end: stage = Learn, loss = 1.2158, val_loss = -1
on_batch_end: stage = Valid, loss = -1, val_loss = 2.94255
```

### Example 2

`state.parameters` gives direct access to the live parameters being
trained -- here used to confirm the optimizer step already ran by
`on_batch_end`, even though it had not yet at `on_batch_begin` for the
same batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class ShowParamChangeCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_begin: param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
  }
  void on_batch_end(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_end:   param[0].sum() = " << state.parameters->front()->sum().item<float>() << "\n";
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
  coach.add_callback(std::make_shared<ShowParamChangeCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_batch_begin: param[0].sum() = -0.599627
on_batch_end:   param[0].sum() = 0.400373
```

### Example 3

A stop requested on the first batch's `on_batch_end` keeps that batch's
already-applied optimizer step, breaks the training loop immediately,
and -- with no validation split here -- prevents any further epoch's
`on_epoch_begin` from firing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopOnFirstBatchEndCallback final : public pluss::ai::ice::Callback {
 public:
  int batch_end_count = 0;
  int epoch_begin_count = 0;
  void on_batch_end(pluss::ai::ice::RunState& state) override {
    ++batch_end_count;
    state.stop_requested = true;
  }
  void on_epoch_begin(pluss::ai::ice::RunState&) override { ++epoch_begin_count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 3;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const auto stopper = std::make_shared<StopOnFirstBatchEndCallback>();
  ice::Coach coach(options);
  coach.add_callback(stopper);
  coach.learn(model, x, y);

  std::cout << "on_batch_end fired " << stopper->batch_end_count << " time(s)\n";
  std::cout << "on_epoch_begin fired " << stopper->epoch_begin_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_batch_end fired 1 time(s)
on_epoch_begin fired 1 time(s)
```

## See Also

- [`on_batch_begin`](on_batch_begin.md) -- the matching hook before the
  batch runs.
- [`RunState::parameters`](../RunState.md#parameters) -- live access to
  the parameters a `Learn` batch's step just updated.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_batch_begin.html">
    <span class="label">Previous</span>
    <span class="title">on_batch_begin</span>
  </a>
  <a class="next-link" href="on_before_backward.html">
    <span class="label">Next</span>
    <span class="title">on_before_backward</span>
  </a>
</div>
<!-- apiref:nav:end -->
