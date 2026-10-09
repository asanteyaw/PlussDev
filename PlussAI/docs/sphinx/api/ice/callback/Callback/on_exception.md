<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce51bac6b7dc -->
# `pluss::ai::ice::Callback::on_exception`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_exception(pluss::ai::ice::RunState& state);
```

## What It Does

Fires once if an exception escapes the run -- in place of
[`on_run_end`](on_run_end.md), never alongside it. The default
implementation does nothing. `Coach::learn`/`infer`/`assess` all catch
`std::exception` around their own run, call this hook, then re-throw
(wrapped with additional context) -- the original failure still
propagates to the caller after every callback's `on_exception` has had
a chance to react. If
[`CheckpointPolicy::save_on_exception`](../../options/CheckpointPolicy.md)
is set, an exception checkpoint write happens right after this hook,
triggering [`on_checkpoint_save`](on_checkpoint_save.md) too.

## Parameters

`RunState& state`
: `state.exception_message` holds the caught exception's own `e.what()`
  text.

## Returns

`void`.

## Errors

None from the base class. If an override itself throws, that new
exception replaces the one already being propagated.

## Practical Examples

Three examples: a real failure (a monitored metric that was never
produced) triggering this hook while the original exception still
reaches the caller, this hook firing alongside
[`on_checkpoint_save`](on_checkpoint_save.md) when
`save_on_exception` is set, and a normal run never triggering it at
all.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogExceptionCallback final : public pluss::ai::ice::Callback {
 public:
  void on_exception(pluss::ai::ice::RunState& state) override {
    std::cout << "on_exception: exception_message is non-empty: " << (!state.exception_message.empty()) << "\n";
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
  // A monitor name that is never produced -- EarlyStopCallback (added
  // automatically, since early_stopping defaults to true) throws on it.
  options.validation.monitor = "not_a_real_metric";

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogExceptionCallback>());

  bool caught = false;
  try {
    coach.learn(model, x, y);
  } catch (const std::exception&) {
    caught = true;
  }
  std::cout << "exception propagated to caller: " << caught << "\n";

  return 0;
}
```

Output

```text
on_exception: exception_message is non-empty: 1
exception propagated to caller: 1
```

### Example 2

With `checkpoint.enabled` and `save_on_exception` both set, the same
failure also writes a `-exception.ckpt` file and fires
`on_checkpoint_save` for it, right after `on_exception`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogExceptionAndCheckpointCallback final : public pluss::ai::ice::Callback {
 public:
  void on_exception(pluss::ai::ice::RunState&) override {
    std::cout << "on_exception fired\n";
  }
  void on_checkpoint_save(pluss::ai::ice::RunState&, std::string_view filepath) override {
    std::cout << "on_checkpoint_save fired: filepath = " << filepath << "\n";
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
  options.validation.monitor = "not_a_real_metric";
  options.checkpoint.enabled = true;
  options.checkpoint.save_on_exception = true;
  options.checkpoint.dirpath = "./ice_checkpoint_demo";
  options.checkpoint.filename = "demo";

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogExceptionAndCheckpointCallback>());

  try {
    coach.learn(model, x, y);
  } catch (const std::exception&) {
    // expected
  }

  return 0;
}
```

Output

```text
on_exception fired
on_checkpoint_save fired: filepath = ./ice_checkpoint_demo/demo-exception.ckpt
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountExceptionCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_exception(pluss::ai::ice::RunState&) override { ++fire_count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 2;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const auto counter = std::make_shared<CountExceptionCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_exception fired " << counter->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_exception fired 0 time(s)
```

## See Also

- [`on_run_end`](on_run_end.md) -- fires instead, on a normal
  completion.
- [`CheckpointPolicy::save_on_exception`](../../options/CheckpointPolicy.md) --
  what triggers the paired checkpoint write.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_epoch_end.html">
    <span class="label">Previous</span>
    <span class="title">on_epoch_end</span>
  </a>
  <a class="next-link" href="on_run_begin.html">
    <span class="label">Next</span>
    <span class="title">on_run_begin</span>
  </a>
</div>
<!-- apiref:nav:end -->
