<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e16271fb09ee -->
# `pluss::ai::ice::Callback::on_epoch_begin`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_epoch_begin(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per epoch, at the very start -- before that epoch's training
batches, before any validation pass. The default implementation does
nothing. This is the one hook whose own `state.stop_requested` check is
checked *before* anything else that epoch runs: setting it here skips
that epoch's entire batch loop, its validation pass, and
[`on_epoch_end`](on_epoch_end.md) outright, going straight to
[`on_run_end`](on_run_end.md) -- a sharper cutoff than requesting a stop
from any other hook mid-epoch (see
[`on_batch_begin`](on_batch_begin.md) for the contrast). Only fires
during `Coach::learn`; `Coach::infer`/`assess` have no epochs.

## Parameters

`RunState& state`
: `state.epoch` is already set to this epoch's index.
  `state.batch_index` is still `-1` and `state.metrics` is empty --
  nothing from this epoch has run yet.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out of
`Coach::learn` and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: the per-epoch firing pattern with `RunState`'s
not-yet-populated fields, the sharper stop-requested cutoff this hook
has compared to the others, and confirming it never fires during
`Coach::infer`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogEpochBeginCallback final : public pluss::ai::ice::Callback {
 public:
  void on_epoch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_epoch_begin: epoch = " << state.epoch
               << ", batch_index = " << state.batch_index
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
  options.execution.max_epochs = 3;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogEpochBeginCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_epoch_begin: epoch = 0, batch_index = -1, metrics.empty() = 1
on_epoch_begin: epoch = 1, batch_index = -1, metrics.empty() = 1
on_epoch_begin: epoch = 2, batch_index = -1, metrics.empty() = 1
```

### Example 2

Requesting a stop from `on_epoch_begin` at epoch `1` skips that
epoch's own [`on_epoch_end`](on_epoch_end.md) entirely -- only epoch
`0`'s `on_epoch_end` ever fires, confirmed by a counter.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopAtEpoch1Callback final : public pluss::ai::ice::Callback {
 public:
  void on_epoch_begin(pluss::ai::ice::RunState& state) override {
    if (state.epoch == 1) state.stop_requested = true;
  }
};

class LogEpochEndCallback final : public pluss::ai::ice::Callback {
 public:
  int epoch_end_count = 0;
  void on_epoch_end(pluss::ai::ice::RunState&) override { ++epoch_end_count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 5;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const auto logger = std::make_shared<LogEpochEndCallback>();
  ice::Coach coach(options);
  coach.add_callback(std::make_shared<StopAtEpoch1Callback>());
  coach.add_callback(logger);
  coach.learn(model, x, y);

  // on_epoch_begin's stop is checked immediately, before that epoch's
  // batch loop or on_epoch_end ever runs -- so epoch 1 never reaches
  // on_epoch_end, and only epoch 0 did.
  std::cout << "on_epoch_end fired " << logger->epoch_end_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_epoch_end fired 1 time(s)
```

### Example 3

`Coach::infer` has no epochs -- `on_epoch_begin` never fires for it,
regardless of how many batches `infer` processes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountEpochBeginCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 0;
  void on_epoch_begin(pluss::ai::ice::RunState&) override { ++count; }
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

  const auto counter = std::make_shared<CountEpochBeginCallback>();
  ice::Coach coach;
  coach.add_callback(counter);
  const auto inferred = coach.infer(model, x);
  static_cast<void>(inferred);

  // Coach::infer has no epochs -- on_epoch_begin never fires for it.
  std::cout << "on_epoch_begin fired " << counter->count << " time(s) during infer\n";

  return 0;
}
```

Output

```text
on_epoch_begin fired 0 time(s) during infer
```

## See Also

- [`on_epoch_end`](on_epoch_end.md) -- the matching hook at the end of
  the epoch, skipped entirely if this hook requests a stop.
- [`on_batch_begin`](on_batch_begin.md) -- the next hook to fire, once
  per batch inside the epoch.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_checkpoint_save.html">
    <span class="label">Previous</span>
    <span class="title">on_checkpoint_save</span>
  </a>
  <a class="next-link" href="on_epoch_end.html">
    <span class="label">Next</span>
    <span class="title">on_epoch_end</span>
  </a>
</div>
<!-- apiref:nav:end -->
