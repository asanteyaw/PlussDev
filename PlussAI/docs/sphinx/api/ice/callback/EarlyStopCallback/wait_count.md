<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e68054331904 -->
# `pluss::ai::ice::EarlyStopCallback::wait_count`

[Back to `pluss::ai::ice::EarlyStopCallback`](../EarlyStopCallback.md)

## Signature

```cpp
[[nodiscard]] std::int64_t pluss::ai::ice::EarlyStopCallback::wait_count() const noexcept;
```

## What It Does

Returns how many validation epochs in a row have failed to improve
[`best_value`](best_value.md). [`on_epoch_end`](../Callback/on_epoch_end.md)
resets this to `0` on every improving epoch, and increments it by `1`
on every non-improving one -- when
[`LoopRegularization::early_stopping`](../../options/LoopRegularization.md)
is `true` and this reaches `patience`, a stop is requested and
[`stopped_epoch`](stopped_epoch.md) is set. With `early_stopping` set
to `false`, this keeps counting exactly the same way, it just never
triggers anything.

## Parameters

None.

## Returns

`std::int64_t` -- `0` before any epoch or right after an improving one,
otherwise the count of consecutive non-improving epochs.

## Errors

None.

## Practical Examples

Three examples: the count climbing toward `patience` and triggering a
stop, the count resetting to `0` on a later improving epoch after
having climbed, and `early_stopping = false` letting the count climb
past `patience` with no effect.

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

Two non-improving epochs raise `wait_count` to `2`, then a later
improving epoch (`0.5`, well below the `1.0` best) resets it straight
back to `0` -- with `patience = 5` here, nowhere near enough to trigger
a stop before that reset happens.

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
  regularization.patience = 5;

  ice::EarlyStopCallback early_stop(validation, regularization);
  ice::RunState state{};
  state.stage = ice::StepStage::Learn;
  early_stop.on_run_begin(state);

  const std::vector<double> val_losses = {1.0, 1.0, 1.0, 0.5};
  for (std::size_t epoch = 0; epoch < val_losses.size(); ++epoch) {
    state.epoch = static_cast<std::int64_t>(epoch);
    state.put_metric("val_loss", val_losses[epoch]);
    early_stop.on_epoch_end(state);
    std::cout << "epoch " << epoch << ": wait_count = " << early_stop.wait_count() << "\n";
  }

  return 0;
}
```

Output

```text
epoch 0: wait_count = 0
epoch 1: wait_count = 1
epoch 2: wait_count = 2
epoch 3: wait_count = 0
```

### Example 3

With `early_stopping = false`, `wait_count` climbs past `patience`
(`1`) with no effect -- `stop_requested` never flips.

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

- [`stopped_epoch`](stopped_epoch.md) -- set once this reaches
  `patience`, with `early_stopping` on.
- [`best_value`](best_value.md) -- what an epoch has to beat to reset
  this to `0`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stopped_epoch.html">
    <span class="label">Previous</span>
    <span class="title">stopped_epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
