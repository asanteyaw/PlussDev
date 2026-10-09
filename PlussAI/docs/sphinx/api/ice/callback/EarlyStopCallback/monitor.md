<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57083a98f020 -->
# `pluss::ai::ice::EarlyStopCallback::monitor`

[Back to `pluss::ai::ice::EarlyStopCallback`](../EarlyStopCallback.md)

## Signature

```cpp
[[nodiscard]] const std::string& pluss::ai::ice::EarlyStopCallback::monitor() const noexcept;
```

## What It Does

Returns the metric name this instance watches -- exactly the
[`ValidationPolicy::monitor`](../../options/ValidationPolicy.md) value
it was constructed with (`validation.monitor`), never updated
afterward. `on_epoch_end` reads
[`RunState::metric(monitor())`](../RunState/metric.md) every validation
epoch against this same fixed name.

## Parameters

None.

## Returns

`const std::string&` -- a reference to the stored monitor name, fixed
for the lifetime of this instance.

## Errors

None.

## Practical Examples

Three examples: reading the default monitor name right after
construction, constructing with a different monitor name, and
confirming it stays fixed across several epochs of `on_epoch_end`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};  // monitor = "val_loss" (the default)
  ice::LoopRegularization regularization{};
  ice::EarlyStopCallback early_stop(validation, regularization);

  std::cout << "monitor() = " << early_stop.monitor() << "\n";

  return 0;
}
```

Output

```text
monitor() = val_loss
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  validation.monitor = "val_accuracy";
  ice::LoopRegularization regularization{};
  ice::EarlyStopCallback early_stop(validation, regularization);

  std::cout << "monitor() = " << early_stop.monitor() << "\n";

  return 0;
}
```

Output

```text
monitor() = val_accuracy
```

### Example 3

`on_epoch_end` reads back `monitor()` itself to know which name to look
up in `RunState` -- here used directly in the driving loop to confirm
it never changes across epochs.

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

  const std::vector<double> val_losses = {1.0, 0.8, 0.8};
  for (std::size_t epoch = 0; epoch < val_losses.size(); ++epoch) {
    state.epoch = static_cast<std::int64_t>(epoch);
    state.put_metric(early_stop.monitor(), val_losses[epoch]);
    early_stop.on_epoch_end(state);
    std::cout << "epoch " << epoch << ": monitor() = " << early_stop.monitor() << " (unchanged)\n";
  }

  return 0;
}
```

Output

```text
epoch 0: monitor() = val_loss (unchanged)
epoch 1: monitor() = val_loss (unchanged)
epoch 2: monitor() = val_loss (unchanged)
```

## See Also

- [`ValidationPolicy::monitor`](../../options/ValidationPolicy.md) --
  the field this returns, and `direction`'s `Auto` inference from its
  name.
- [`best_value`](best_value.md) -- the best value seen so far for this
  same metric.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="best_value.html">
    <span class="label">Previous</span>
    <span class="title">best_value</span>
  </a>
  <a class="next-link" href="stopped_epoch.html">
    <span class="label">Next</span>
    <span class="title">stopped_epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
