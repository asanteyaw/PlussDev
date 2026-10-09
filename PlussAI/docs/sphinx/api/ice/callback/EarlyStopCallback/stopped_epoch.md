<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 12a1d146c465 -->
# `pluss::ai::ice::EarlyStopCallback::stopped_epoch`

[Back to `pluss::ai::ice::EarlyStopCallback`](../EarlyStopCallback.md)

## Signature

```cpp
[[nodiscard]] std::int64_t pluss::ai::ice::EarlyStopCallback::stopped_epoch() const noexcept;
```

## What It Does

Returns the epoch where [`wait_count`](wait_count.md) actually reached
[`LoopRegularization::patience`](../../options/LoopRegularization.md)
and a stop was requested -- set only inside
[`on_epoch_end`](../Callback/on_epoch_end.md), only when
`LoopRegularization::early_stopping` is `true`. With `early_stopping`
set to `false`, `wait_count` is still tracked every epoch but this
value never changes, no matter how far past `patience` it climbs.

## Parameters

None.

## Returns

`std::int64_t` -- `-1` if no stop has been requested yet (or
`early_stopping` is `false`), the `0`-based epoch index where
`RunState::stop_requested` was set otherwise.

## Errors

None.

## Practical Examples

Three examples: a `Min`-direction run that stops once `patience` is
exhausted, a `Max`-direction run stopping with a tighter `patience`,
and `early_stopping = false` leaving this at `-1` even as `wait_count`
keeps climbing.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  ice::LoopRegularization regularization{};
  regularization.early_stopping = true;
  regularization.patience = 2;

  ice::EarlyStopCallback early_stop(validation, regularization);
  ice::RunState state{};
  state.stage = ice::StepStage::Learn;
  early_stop.on_run_begin(state);

  const std::vector<double> val_losses = {1.0, 0.8, 0.8, 0.8, 0.8};
  for (std::size_t epoch = 0; epoch < val_losses.size(); ++epoch) {
    state.epoch = static_cast<std::int64_t>(epoch);
    state.put_metric("val_loss", val_losses[epoch]);
    early_stop.on_epoch_end(state);
    std::cout << "epoch " << epoch << ": wait_count = " << early_stop.wait_count()
               << ", best_value = " << early_stop.best_value().value_or(-1.0)
               << ", stop_requested = " << state.stop_requested << "\n";
    if (state.stop_requested) break;
  }
  std::cout << "stopped_epoch = " << early_stop.stopped_epoch() << "\n";
  std::cout << "best_epoch = " << early_stop.best_epoch() << "\n";
  std::cout << "monitor = " << early_stop.monitor() << "\n";

  return 0;
}
```

Output

```text
epoch 0: wait_count = 0, best_value = 1, stop_requested = 0
epoch 1: wait_count = 0, best_value = 0.8, stop_requested = 0
epoch 2: wait_count = 1, best_value = 0.8, stop_requested = 0
epoch 3: wait_count = 2, best_value = 0.8, stop_requested = 1
stopped_epoch = 3
best_epoch = 1
monitor = val_loss
```

### Example 2

A tighter `patience = 1` under `Max` direction stops one epoch sooner,
right after the first non-improving epoch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  validation.monitor = "val_accuracy";
  ice::LoopRegularization regularization{};
  regularization.early_stopping = true;
  regularization.patience = 1;

  ice::EarlyStopCallback early_stop(validation, regularization);
  ice::RunState state{};
  state.stage = ice::StepStage::Learn;
  early_stop.on_run_begin(state);

  const std::vector<double> val_accuracy = {0.5, 0.6, 0.55, 0.52};
  for (std::size_t epoch = 0; epoch < val_accuracy.size(); ++epoch) {
    state.epoch = static_cast<std::int64_t>(epoch);
    state.put_metric("val_accuracy", val_accuracy[epoch]);
    early_stop.on_epoch_end(state);
    if (state.stop_requested) break;
  }
  std::cout << "stopped_epoch() = " << early_stop.stopped_epoch() << "\n";

  return 0;
}
```

Output

```text
stopped_epoch() = 2
```

### Example 3

With `early_stopping = false`, `wait_count` still climbs past
`patience`, but `stopped_epoch()` never changes -- nothing ever
requests a stop.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  ice::LoopRegularization regularization{};
  regularization.early_stopping = false;
  regularization.patience = 1;

  ice::EarlyStopCallback early_stop(validation, regularization);
  ice::RunState state{};
  state.stage = ice::StepStage::Learn;
  early_stop.on_run_begin(state);

  const std::vector<double> val_losses = {1.0, 0.8, 0.8, 0.8};
  for (std::size_t epoch = 0; epoch < val_losses.size(); ++epoch) {
    state.epoch = static_cast<std::int64_t>(epoch);
    state.put_metric("val_loss", val_losses[epoch]);
    early_stop.on_epoch_end(state);
    std::cout << "epoch " << epoch << ": wait_count() = " << early_stop.wait_count()
               << ", stop_requested = " << state.stop_requested
               << ", stopped_epoch() = " << early_stop.stopped_epoch() << "\n";
  }

  return 0;
}
```

Output

```text
epoch 0: wait_count() = 0, stop_requested = 0, stopped_epoch() = -1
epoch 1: wait_count() = 0, stop_requested = 0, stopped_epoch() = -1
epoch 2: wait_count() = 1, stop_requested = 0, stopped_epoch() = -1
epoch 3: wait_count() = 2, stop_requested = 0, stopped_epoch() = -1
```

## See Also

- [`wait_count`](wait_count.md) -- what has to reach `patience` for
  this to change.
- [`LoopRegularization::early_stopping`](../../options/LoopRegularization.md) --
  the gate on whether a stop is ever requested at all.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="monitor.html">
    <span class="label">Previous</span>
    <span class="title">monitor</span>
  </a>
  <a class="next-link" href="wait_count.html">
    <span class="label">Next</span>
    <span class="title">wait_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
