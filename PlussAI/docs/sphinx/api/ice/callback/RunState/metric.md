<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9727642cc8bb -->
# `pluss::ai::ice::RunState::metric`

[Back to `pluss::ai::ice::RunState`](../RunState.md)

## Signature

```cpp
[[nodiscard]] std::optional<double> pluss::ai::ice::RunState::metric(std::string_view name) const;
```

## What It Does

Reads a named entry out of [`metrics`](../RunState.md#metrics) --
`"loss"` is always present once a batch has run, `"val_loss"` once a
validation pass has run. Prefer this over indexing `state.metrics`
directly: a missing key reads as `std::nullopt` instead of
default-constructing (and silently inserting) a `0.0` entry the way
`metrics["missing"]` would.

## Parameters

`std::string_view name`
: The metric's name -- `"loss"`/`"val_loss"` for the two `Coach::learn`
  always produces, or any name a [`Callback`](../Callback.md) wrote
  itself via [`put_metric`](put_metric.md).

## Returns

`std::optional<double>` -- the value if `name` is present in `metrics`
right now, `std::nullopt` otherwise. Remember that `metrics` is cleared
at the start of every batch and every epoch (see
[`RunState::metrics`](../RunState.md#metrics)), so which names are
present depends on exactly when a hook reads it.

## Errors

None -- a missing name is a normal, expected outcome, not a failure.

## Practical Examples

Three examples: a direct call against a hand-built `RunState`, reading
`"loss"` from inside a real training batch, and reading both `"loss"`
and `"val_loss"` once validation has run.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::RunState state{};
  state.put_metric("loss", 1.5);

  std::cout << "metric(\"loss\").has_value()    = " << state.metric("loss").has_value() << "\n";
  std::cout << "metric(\"loss\").value()         = " << state.metric("loss").value() << "\n";
  std::cout << "metric(\"missing\").has_value()  = " << state.metric("missing").has_value() << "\n";

  return 0;
}
```

Output

```text
metric("loss").has_value()    = 1
metric("loss").value()         = 1.5
metric("missing").has_value()  = 0
```

### Example 2

`Coach::learn` puts `"loss"` into `metrics` right before
[`on_before_backward`](../Callback/on_before_backward.md) fires, so any
hook from that point through [`on_batch_end`](../Callback/on_batch_end.md)
can read it -- here read back inside a custom `Callback`'s own
`on_batch_end`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class PrintLossCallback final : public pluss::ai::ice::Callback {
 public:
  void on_batch_end(pluss::ai::ice::RunState& state) override {
    const auto loss = state.metric("loss");
    if (loss.has_value()) {
      std::cout << "on_batch_end: loss = " << *loss << "\n";
    }
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
  coach.add_callback(std::make_shared<PrintLossCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_batch_end: loss = 2.35997
```

### Example 3

With a validation split, `on_epoch_end` sees both the epoch's averaged
training `"loss"` and the validation pass's `"val_loss"` --
`Coach::learn` resets `state.stage` back to `Learn` and repopulates
`metrics` with both right before `on_epoch_end` fires.

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

## See Also

- [`put_metric`](put_metric.md) -- the write side.
- [`RunState::metrics`](../RunState.md#metrics) -- the underlying map,
  and exactly when it is cleared.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="put_metric.html">
    <span class="label">Next</span>
    <span class="title">put_metric</span>
  </a>
</div>
<!-- apiref:nav:end -->
