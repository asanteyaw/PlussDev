<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d3d6241e482c -->
# `pluss::ai::ice::Callback::on_epoch_end`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_epoch_end(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once per epoch, after that epoch's training batches and (if this
epoch runs one) its validation pass -- `state.stage` is reset to
`Learn` and `state.metrics` repopulated with `"loss"`/`"val_loss"`
before this fires, regardless of what stage the last batch ran under.
The default implementation does nothing.
[`EarlyStopCallback`](../EarlyStopCallback.md) overrides this to
implement its own stopping logic -- see its
[Introduction](../EarlyStopCallback/Introduction.md) for the algorithm.
Unlike [`on_epoch_begin`](on_epoch_begin.md), a stop requested here does
not cut this epoch short (it already finished); it only prevents the
*next* epoch's `on_epoch_begin` from firing.

## Parameters

`RunState& state`
: `state.metrics` holds this epoch's averaged `"loss"` and, if
  validation ran, `"val_loss"` --
  read them via [`RunState::metric`](../RunState/metric.md).

## Returns

`void`.

## Errors

None from the base class; whatever an override throws (as
`EarlyStopCallback` does when its monitored metric is missing)
propagates out of `Coach::learn` and triggers
[`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: reading the epoch's metrics back, confirming hooks fire
on multiple callbacks in the order they were added, and a stop
requested here only taking effect before the *next* epoch.

### Example 1

With a validation split, `on_epoch_end` sees both the epoch's averaged
training `"loss"` and the validation pass's `"val_loss"`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class PrintEpochMetricsCallback final : public pluss::ai::ice::Callback {
 public:
  void on_epoch_end(pluss::ai::ice::RunState& state) override {
    const auto loss = state.metric("loss");
    const auto val_loss = state.metric("val_loss");
    std::cout << "epoch " << state.epoch << ": loss = " << loss.value_or(-1.0)
               << ", val_loss = " << val_loss.value_or(-1.0) << "\n";
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
  coach.add_callback(std::make_shared<PrintEpochMetricsCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
epoch 0: loss = 1.2158, val_loss = 2.94255
```

### Example 2

Two callbacks both override `on_epoch_end` -- each fires in the order
it was added via [`Coach::add_callback`](../../coach/Coach.md). This is
also the ordering `Coach` relies on internally: a built-in
`EarlyStopCallback` is always appended *after* whatever callbacks the
caller added, so a user callback's own `on_epoch_end` always runs
before early-stopping's own check that same epoch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class TagEpochEndCallback final : public pluss::ai::ice::Callback {
 public:
  explicit TagEpochEndCallback(std::string tag) : tag_(std::move(tag)) {}
  void on_epoch_end(pluss::ai::ice::RunState& state) override {
    std::cout << "epoch " << state.epoch << ": on_epoch_end from " << tag_ << "\n";
  }

 private:
  std::string tag_;
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

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<TagEpochEndCallback>("first"));
  coach.add_callback(std::make_shared<TagEpochEndCallback>("second"));
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
epoch 0: on_epoch_end from first
epoch 0: on_epoch_end from second
```

### Example 3

Requesting a stop from `on_epoch_end` at epoch `1` does not cut epoch
`1` itself short (it already ran to completion) -- it only prevents
epoch `2`'s `on_epoch_begin` from firing. Both epoch `0` and epoch `1`
still reach `on_epoch_begin`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StopAtEpoch1Callback final : public pluss::ai::ice::Callback {
 public:
  void on_epoch_end(pluss::ai::ice::RunState& state) override {
    if (state.epoch == 1) state.stop_requested = true;
  }
};

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
  options.execution.max_epochs = 5;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const auto counter = std::make_shared<CountEpochBeginCallback>();
  ice::Coach coach(options);
  coach.add_callback(std::make_shared<StopAtEpoch1Callback>());
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_epoch_begin fired " << counter->count << " time(s)\n";

  return 0;
}
```

Output

```text
on_epoch_begin fired 2 time(s)
```

## See Also

- [`on_epoch_begin`](on_epoch_begin.md) -- the matching hook at the
  start of the epoch; a stop requested there has a sharper, immediate
  cutoff than one requested here.
- [`RunState::metric`](../RunState/metric.md) -- how to read the
  metrics available at this point.
- [`EarlyStopCallback`](../EarlyStopCallback.md) -- the built-in
  override, always appended after any caller-added callbacks.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_epoch_begin.html">
    <span class="label">Previous</span>
    <span class="title">on_epoch_begin</span>
  </a>
  <a class="next-link" href="on_exception.html">
    <span class="label">Next</span>
    <span class="title">on_exception</span>
  </a>
</div>
<!-- apiref:nav:end -->
