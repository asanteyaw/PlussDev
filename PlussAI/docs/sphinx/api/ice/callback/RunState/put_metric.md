<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 464614da37bb -->
# `pluss::ai::ice::RunState::put_metric`

[Back to `pluss::ai::ice::RunState`](../RunState.md)

## Signature

```cpp
void pluss::ai::ice::RunState::put_metric(std::string name, double value);
```

## What It Does

Writes or overwrites one named entry in
[`metrics`](../RunState.md#metrics). `Coach::learn` calls this itself
for `"loss"`/`"val_loss"`; a custom [`Callback`](../Callback.md) calls
it from any hook to publish its own named value -- a learning-rate
schedule's current rate, a gradient norm, anything worth surfacing
alongside the built-in metrics for the rest of the run to read (a
logging callback, [`EarlyStopCallback`](../EarlyStopCallback.md), or a
checkpoint policy monitoring that name).

## Parameters

`std::string name`
: The metric's key. Reusing `"loss"`/`"val_loss"` overwrites what
  `Coach::learn` itself wrote for the current batch/epoch; any other
  name adds a new entry alongside them.

`double value`
: The value to store.

## Returns

`void` -- mutates `metrics` in place.

## Errors

None.

## Practical Examples

Three examples: a direct call against a hand-built `RunState`,
publishing a custom metric from inside a real training batch, and
[`EarlyStopCallback`](../EarlyStopCallback.md)'s own monitored metric
read back by name later in the same run.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::RunState state{};
  std::cout << "before: has_value() = " << state.metric("grad_norm").has_value() << "\n";

  state.put_metric("grad_norm", 0.42);
  std::cout << "after:  has_value() = " << state.metric("grad_norm").has_value()
             << ", value = " << state.metric("grad_norm").value() << "\n";

  state.put_metric("grad_norm", 0.17);
  std::cout << "overwritten: value = " << state.metric("grad_norm").value() << "\n";

  return 0;
}
```

Output

```text
before: has_value() = 0
after:  has_value() = 1, value = 0.42
overwritten: value = 0.17
```

### Example 2

A custom `Callback` publishes its own metric -- here a running batch
counter -- alongside `"loss"`, from inside
[`on_batch_end`](../Callback/on_batch_end.md). Nothing about `Coach`
needs to know this name in advance; any later hook (in this same run or
a chained callback) can read it back through
[`metric`](metric.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class StepCounterCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_end(pluss::ai::ice::RunState& state) override {
    state.put_metric("steps_seen", static_cast<double>(++steps_));
    std::cout << "steps_seen = " << state.metric("steps_seen").value()
               << ", loss = " << state.metric("loss").value_or(-1.0) << "\n";
  }

 private:
  int steps_ = 0;
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
  coach.add_callback(std::make_shared<StepCounterCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
steps_seen = 1, loss = 2.35997
```

### Example 3

`Coach::learn` itself calls `put_metric` for `"loss"` right before
[`on_before_backward`](../Callback/on_before_backward.md), and again
for `"loss"`/`"val_loss"` right before
[`on_epoch_end`](../Callback/on_epoch_end.md) once validation has run
-- the same mechanism a custom callback uses, just called from inside
`Coach` itself.

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
    std::cout << "epoch " << state.epoch << ": loss = " << state.metric("loss").value_or(-1.0)
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
  coach.add_callback(std::make_shared<PrintEpochMetricsCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
epoch 0: loss = 1.2158, val_loss = 2.94255
```

## See Also

- [`metric`](metric.md) -- the read side.
- [`RunState::metrics`](../RunState.md#metrics) -- the underlying map,
  and exactly when it is cleared.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="metric.html">
    <span class="label">Previous</span>
    <span class="title">metric</span>
  </a>
</div>
<!-- apiref:nav:end -->
