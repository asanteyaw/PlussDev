<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bab799153eaa -->
# `pluss::ai::ice::Callback::on_run_end`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_run_end(pluss::ai::ice::RunState& state);
```

## What It Does

Fires exactly once, at the very end of a `Coach::learn`/`infer`/
`assess` run -- whether it ended naturally (`max_epochs` reached, or
every batch processed) or early, because some hook set
`state.stop_requested`. The default implementation does nothing.
Override it for one-time teardown: closing a log file, printing a
summary, or reading `state.stop_requested`/`state.stop_reason` to know
why the run ended. [`EarlyStopCallback`](../EarlyStopCallback.md)
overrides this specifically to restore the best-seen parameters when
`restore_best_weights` is set.

## Parameters

`RunState& state`
: `state.epoch` is the last epoch that ran (not necessarily `max_epochs
  - 1`, if the run stopped early). `state.stop_requested`/
  `state.stop_reason` report whether and why a hook ended the run early.

## Returns

`void`.

## Errors

None from the base class; `on_run_end` does not fire at all if an
exception propagated out of the run -- see
[`on_exception`](on_exception.md) for that path instead.

## Practical Examples

Three examples: the natural end-of-run case, a callback that stops the
run early from [`on_epoch_begin`](on_epoch_begin.md), and confirming
this hook also fires for `Coach::infer`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogRunEndCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "on_run_end: epoch = " << state.epoch
               << ", stop_requested = " << state.stop_requested << "\n";
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
  coach.add_callback(std::make_shared<LogRunEndCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_run_end: epoch = 2, stop_requested = 0
```

### Example 2

A callback requests a stop from
[`on_epoch_begin`](on_epoch_begin.md) at epoch `1`; `on_run_end` still
fires, with `state.epoch` left at `1` (the epoch mid-flight when the
stop was requested) and `stop_reason` carrying whatever the stopping
callback wrote.

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
    if (state.epoch == 1) {
      state.stop_requested = true;
      state.stop_reason = "manual_stop";
    }
  }
};

class LogRunEndCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "on_run_end: epoch = " << state.epoch
               << ", stop_reason = \"" << state.stop_reason << "\"\n";
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
  options.execution.max_epochs = 5;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<StopAtEpoch1Callback>());
  coach.add_callback(std::make_shared<LogRunEndCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_run_end: epoch = 1, stop_reason = "manual_stop"
```

### Example 3

`on_run_end` fires for `Coach::infer` too, with `state.stage` left at
`Infer`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogRunEndCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_end(pluss::ai::ice::RunState& state) override {
    std::cout << "on_run_end: stage = " << static_cast<int>(state.stage) << "\n";
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
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogRunEndCallback>());
  coach.learn(model, x, y);
  const auto inferred = coach.infer(model, x);
  static_cast<void>(inferred);

  return 0;
}
```

Output

```text
on_run_end: stage = 0
on_run_end: stage = 3
```

## See Also

- [`on_run_begin`](on_run_begin.md) -- the matching hook at the start
  of the run.
- [`on_exception`](on_exception.md) -- fires instead, if the run ended
  via an uncaught exception.
- [`EarlyStopCallback`](../EarlyStopCallback.md) -- the built-in
  override that restores best-seen parameters here.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_run_begin.html">
    <span class="label">Previous</span>
    <span class="title">on_run_begin</span>
  </a>
  <a class="next-link" href="save_state.html">
    <span class="label">Next</span>
    <span class="title">save_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
