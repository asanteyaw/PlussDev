<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 77ba5c01a81c -->
# `pluss::ai::ice::EarlyStopCallback::best_value`

[Back to `pluss::ai::ice::EarlyStopCallback`](../EarlyStopCallback.md)

## Signature

```cpp
[[nodiscard]] std::optional<double> pluss::ai::ice::EarlyStopCallback::best_value() const noexcept;
```

## What It Does

Returns the best value [`monitor`](monitor.md) has reached so far this
run, per [`ValidationPolicy::direction`](../../options/ValidationPolicy.md)
(lower is better for `Min`, higher for `Max`). Updated by
[`on_epoch_end`](../Callback/on_epoch_end.md) every time the monitored
metric improves by at least
[`LoopRegularization::min_delta`](../../options/LoopRegularization.md);
left unchanged on a non-improving epoch, which is also what increments
[`wait_count`](wait_count.md).

## Parameters

None.

## Returns

`std::optional<double>` -- `std::nullopt` before the first validation
epoch has run (there is nothing to compare yet), the best value
afterward.

## Errors

None.

## Practical Examples

Three examples: the `std::nullopt` starting state, a `Min`-direction
run where the value improves once and then plateaus, and a
`Max`-direction run (an accuracy-like metric) showing the comparison
flips.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  ice::LoopRegularization regularization{};
  ice::EarlyStopCallback early_stop(validation, regularization);

  std::cout << "before any epoch: best_value().has_value() = " << early_stop.best_value().has_value()
             << ", best_epoch() = " << early_stop.best_epoch()
             << ", stopped_epoch() = " << early_stop.stopped_epoch()
             << ", wait_count() = " << early_stop.wait_count() << "\n";

  return 0;
}
```

Output

```text
before any epoch: best_value().has_value() = 0, best_epoch() = -1, stopped_epoch() = -1, wait_count() = 0
```

### Example 2

`"val_loss"` improves from `1.0` to `0.8` at epoch 1, then plateaus --
`best_value()` tracks `0.8` from that point on, even as `wait_count`
keeps climbing on the later non-improving epochs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};  // monitor = "val_loss", direction = Auto (-> Min)
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

### Example 3

`"val_accuracy"` is inferred as `Max` direction (its name contains
`"acc"`) -- `best_value()` only updates when accuracy goes *up*.

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
    std::cout << "epoch " << epoch << ": best_value = " << early_stop.best_value().value_or(-1.0)
               << ", wait_count = " << early_stop.wait_count()
               << ", stop_requested = " << state.stop_requested << "\n";
    if (state.stop_requested) break;
  }

  return 0;
}
```

Output

```text
epoch 0: best_value = 0.5, wait_count = 0, stop_requested = 0
epoch 1: best_value = 0.6, wait_count = 0, stop_requested = 0
epoch 2: best_value = 0.6, wait_count = 1, stop_requested = 1
```

## See Also

- [`monitor`](monitor.md) -- which metric this value is tracking.
- [`best_epoch`](best_epoch.md) -- which epoch produced this value.
- [`wait_count`](wait_count.md) -- how many epochs in a row have failed
  to beat this value.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="best_epoch.html">
    <span class="label">Previous</span>
    <span class="title">best_epoch</span>
  </a>
  <a class="next-link" href="monitor.html">
    <span class="label">Next</span>
    <span class="title">monitor</span>
  </a>
</div>
<!-- apiref:nav:end -->
