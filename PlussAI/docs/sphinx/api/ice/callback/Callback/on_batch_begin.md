<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 10df291706f7 -->
# `pluss::ai::ice::Callback::on_batch_begin`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_batch_begin(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per batch, right before that batch is transferred and run
through the model -- during `Coach::learn`, for both training batches
(`state.stage == Learn`) and, if the epoch runs one, every validation
batch (`state.stage == Valid`) too. Also fires once per batch during
`Coach::infer`/`assess` (`state.stage == Infer`/`Test`). The default
implementation does nothing.

A stop requested here breaks the *current* batch loop immediately --
the training loop if `state.stage == Learn`, the validation loop if
`state.stage == Valid` -- but does not skip anything that loop's own
caller does next. In particular: requesting a stop during the *first*
training batch of an epoch that also validates still lets that epoch's
validation pass begin (its `on_batch_begin` fires too, once, before the
already-true stop is checked again and breaks that loop as well) --
and because no validation batch actually ran, `Coach::learn` raises its
own "no validation rows" error rather than completing the epoch. Stop
from [`on_epoch_begin`](on_epoch_begin.md) instead for a clean cutoff
that skips validation entirely.

## Parameters

`RunState& state`
: `state.rows` is already set to this batch's row count;
  `state.metrics` is empty (cleared right before this fires).

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: the stage/row/metrics state visible at this point
across a training epoch with validation, the batch-loop-only cutoff a
stop requested here produces, and confirming it fires during
`Coach::infer` too.

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

class LogBatchBeginCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_begin: stage = " << stage_name(state.stage)
               << ", batch_index = " << state.batch_index
               << ", rows = " << state.rows
               << ", metrics.empty() = " << state.metrics.empty() << "\n";
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
  coach.add_callback(std::make_shared<LogBatchBeginCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_batch_begin: stage = Learn, batch_index = 0, rows = 2, metrics.empty() = 1
on_batch_begin: stage = Valid, batch_index = 0, rows = 2, metrics.empty() = 1
```

### Example 2

With no validation split, a stop requested on the very first training
batch breaks that batch loop cleanly -- no further batches that epoch,
and no epoch beyond `0` ever starts.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopOnFirstBatchCallback final : public pluss::ai::ice::Callback {
 public:
  int batch_begin_count = 0;
  int epoch_begin_count = 0;
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    ++batch_begin_count;
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

  const auto stopper = std::make_shared<StopOnFirstBatchCallback>();
  ice::Coach coach(options);
  coach.add_callback(stopper);
  coach.learn(model, x, y);

  std::cout << "on_batch_begin fired " << stopper->batch_begin_count << " time(s)\n";
  std::cout << "on_epoch_begin fired " << stopper->epoch_begin_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_batch_begin fired 1 time(s)
on_epoch_begin fired 1 time(s)
```

### Example 3

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

class LogBatchBeginCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_batch_begin: stage = " << stage_name(state.stage)
               << ", batch_index = " << state.batch_index << "\n";
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
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  ice::Coach coach;
  coach.add_callback(std::make_shared<LogBatchBeginCallback>());
  const auto inferred = coach.infer(model, x);
  static_cast<void>(inferred);

  return 0;
}
```

Output

```text
on_batch_begin: stage = Infer, batch_index = 0
```

## See Also

- [`on_batch_end`](on_batch_end.md) -- the matching hook after the
  batch runs.
- [`on_epoch_begin`](on_epoch_begin.md) -- stop from there instead for
  a clean, validation-skipping cutoff.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_after_backward.html">
    <span class="label">Previous</span>
    <span class="title">on_after_backward</span>
  </a>
  <a class="next-link" href="on_batch_end.html">
    <span class="label">Next</span>
    <span class="title">on_batch_end</span>
  </a>
</div>
<!-- apiref:nav:end -->
