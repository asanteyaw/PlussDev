<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a92bd89ebd3b -->
# `pluss::ai::ice::EarlyStopCallback::best_epoch`

[Back to `pluss::ai::ice::EarlyStopCallback`](../EarlyStopCallback.md)

## Signature

```cpp
[[nodiscard]] std::int64_t pluss::ai::ice::EarlyStopCallback::best_epoch() const noexcept;
```

## What It Does

Returns the epoch index that produced [`best_value`](best_value.md) --
the last epoch where the monitored metric actually improved, not
necessarily the most recent epoch. Updated in the same step as
`best_value` inside [`on_epoch_end`](../Callback/on_epoch_end.md): both
change together, exactly on an improving epoch, and both stay put on a
non-improving one.

## Parameters

None.

## Returns

`std::int64_t` -- `-1` before any validation epoch has improved on the
starting state, the `0`-based epoch index of the best epoch afterward.

## Errors

None.

## Practical Examples

Three examples: the `-1` starting state, a run where the best epoch is
reached early and every later epoch just plateaus around it, and a
`Max`-direction run showing the same freeze-on-plateau behavior.

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

The monitored loss improves at epoch 1 and never improves again --
`best_epoch()` stays `1` through epochs 2 and 3, even though training
keeps running until `wait_count` reaches `patience` at epoch 3.

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
    std::cout << "epoch " << epoch << ": best_epoch() = " << early_stop.best_epoch()
               << ", stop_requested = " << state.stop_requested << "\n";
    if (state.stop_requested) break;
  }

  return 0;
}
```

Output

```text
epoch 0: best_epoch() = 0, stop_requested = 0
epoch 1: best_epoch() = 1, stop_requested = 0
epoch 2: best_epoch() = 1, stop_requested = 0
epoch 3: best_epoch() = 1, stop_requested = 1
```

### Example 3

The same freeze-on-plateau behavior under `Max` direction: accuracy
peaks at epoch 1, and `best_epoch()` stays there once accuracy starts
drifting back down.

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
    std::cout << "epoch " << epoch << ": best_epoch() = " << early_stop.best_epoch()
               << ", stop_requested = " << state.stop_requested << "\n";
    if (state.stop_requested) break;
  }

  return 0;
}
```

Output

```text
epoch 0: best_epoch() = 0, stop_requested = 0
epoch 1: best_epoch() = 1, stop_requested = 0
epoch 2: best_epoch() = 1, stop_requested = 1
```

## See Also

- [`best_value`](best_value.md) -- the value reached at this epoch.
- [`stopped_epoch`](stopped_epoch.md) -- the (usually later) epoch
  where `patience` actually ran out.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="best_value.html">
    <span class="label">Next</span>
    <span class="title">best_value</span>
  </a>
</div>
<!-- apiref:nav:end -->
