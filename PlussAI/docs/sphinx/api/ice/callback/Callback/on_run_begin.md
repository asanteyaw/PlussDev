<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b8afc715f339 -->
# `pluss::ai::ice::Callback::on_run_begin`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_run_begin(pluss::ai::ice::RunState& state);
```

## What It Does

Fires exactly once, at the very start of a `Coach::learn`/`infer`/
`assess` run -- before the first epoch (`learn`) or the first batch
(`infer`/`assess`). The default implementation does nothing. Override
it for one-time setup: opening a log file, resetting a counter, or
reading `state.recipe`/`state.parameters` to configure something before
the run proper begins.

## Parameters

`RunState& state`
: `state.model`/`state.parameters` are already set (for `learn`;
  `state.recipe` too). `state.epoch`/`state.batch_index` are still at
  their starting sentinels (`-1`), since nothing has run yet.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out of
`Coach::learn`/`infer`/`assess` and triggers
[`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: reading `RunState`'s already-populated fields at this
point, confirming it fires exactly once regardless of epoch count, and
the fact that it fires for `Coach::infer` too, not only `learn`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogRunBeginCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_run_begin: epoch = " << state.epoch
               << ", model != nullptr: " << (state.model != nullptr)
               << ", parameters != nullptr: " << (state.parameters != nullptr)
               << ", recipe != nullptr: " << (state.recipe != nullptr) << "\n";
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
  options.execution.max_epochs = 2;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogRunBeginCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_run_begin: epoch = -1, model != nullptr: 1, parameters != nullptr: 1, recipe != nullptr: 1
```

### Example 2

Regardless of how many epochs `learn` runs, `on_run_begin` fires once,
at the very start.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountRunBeginCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 0;
  void on_run_begin(pluss::ai::ice::RunState&) override { ++count; }
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

  const auto counter = std::make_shared<CountRunBeginCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_run_begin fired " << counter->count << " time(s) across " << options.execution.max_epochs << " epochs\n";

  return 0;
}
```

Output

```text
on_run_begin fired 1 time(s) across 5 epochs
```

### Example 3

`on_run_begin` fires for `Coach::infer` too -- a separate run, with
`state.stage` already at `Infer` and (unlike `learn`) `state.recipe`
left at `nullptr`, since inference has no `TrainOptions` recipe driving
it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogRunBeginCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_run_begin: stage = " << static_cast<int>(state.stage)
               << ", recipe == nullptr: " << (state.recipe == nullptr) << "\n";
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
  coach.add_callback(std::make_shared<LogRunBeginCallback>());
  coach.learn(model, x, y);

  // on_run_begin also fires for Coach::infer, a separate run with no
  // training-specific RunState fields populated.
  const auto inferred = coach.infer(model, x);
  static_cast<void>(inferred);

  return 0;
}
```

Output

```text
on_run_begin: stage = 0, recipe == nullptr: 0
on_run_begin: stage = 3, recipe == nullptr: 1
```

## See Also

- [`on_run_end`](on_run_end.md) -- the matching hook at the other end
  of the run.
- [`RunState`](../RunState.md) -- every field available at this point.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_exception.html">
    <span class="label">Previous</span>
    <span class="title">on_exception</span>
  </a>
  <a class="next-link" href="on_run_end.html">
    <span class="label">Next</span>
    <span class="title">on_run_end</span>
  </a>
</div>
<!-- apiref:nav:end -->
