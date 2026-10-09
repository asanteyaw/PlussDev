<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0b4dd2b90555 -->
# `pluss::ai::ice::Callback::load_state`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::load_state(const std::unordered_map<std::string, std::string>& state);
```

## What It Does

Restores this callback's own state from the map
[`save_state`](save_state.md) previously produced. `Coach::learn` calls
this once per callback, for every callback whose
[`state_key`](state_key.md) is non-empty and present in the resumed
checkpoint's own stored state, right after the checkpoint file is read
-- before [`on_checkpoint_load`](on_checkpoint_load.md) fires. The
default implementation does nothing. An override only needs to parse
back whatever `save_state` wrote; the map's values are always plain
strings regardless of a field's real type.

## Parameters

`const std::unordered_map<std::string, std::string>& state`
: The same map this callback's own `save_state` produced on a previous
  run, read back from the checkpoint.

## Returns

`void`.

## Errors

None from the base class; an override that mis-parses a value (for
example `std::stoi` on malformed text) throws whatever that parse
itself throws.

## Practical Examples

Three examples: the default no-op accepting arbitrary data safely, a
custom override restoring a member from a hand-built map, and this
method actually called by `Coach::learn` on a real checkpoint resume.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <unordered_map>

namespace {

class PlainCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  PlainCallback callback;
  const std::unordered_map<std::string, std::string> some_state{{"count", "42"}};

  // The default implementation ignores whatever is passed -- there is
  // nothing to restore and no observable effect.
  callback.load_state(some_state);
  std::cout << "load_state() on the default implementation has no observable effect\n";

  return 0;
}
```

Output

```text
load_state() on the default implementation has no observable effect
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <unordered_map>

namespace {

class EpochCounterCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 0;
  void load_state(const std::unordered_map<std::string, std::string>& state) override {
    const auto it = state.find("count");
    if (it != state.end()) count = std::stoi(it->second);
  }
};

} // namespace

int main() {
  EpochCounterCallback callback;
  std::cout << "before: count = " << callback.count << "\n";

  callback.load_state({{"count", "7"}});
  std::cout << "after:  count = " << callback.count << "\n";

  return 0;
}
```

Output

```text
before: count = 0
after:  count = 7
```

### Example 3

`Coach::learn` calls `load_state` itself during a checkpoint resume --
here it is what lets a *fresh* `EpochCounterCallback` instance, added
to a new `Coach`, pick its count back up from the first run's final
value instead of starting over at `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

class EpochCounterCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 0;

  std::string state_key() const override { return "epoch_counter"; }

  std::unordered_map<std::string, std::string> save_state() const override {
    return {{"count", std::to_string(count)}};
  }

  void load_state(const std::unordered_map<std::string, std::string>& state) override {
    const auto it = state.find("count");
    if (it != state.end()) count = std::stoi(it->second);
  }

  void on_epoch_end(pluss::ai::ice::RunState&) override { ++count; }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions save_options{};
  save_options.execution.max_epochs = 2;
  save_options.learning_rate = 0.1;
  save_options.validation.split_ratio = 0.0;
  save_options.checkpoint.enabled = true;
  save_options.checkpoint.save_last = true;
  save_options.checkpoint.dirpath = "./ice_checkpoint_demo";
  save_options.checkpoint.filename = "demo";

  const auto first_counter = std::make_shared<EpochCounterCallback>();
  ice::Coach save_coach(save_options);
  save_coach.add_callback(first_counter);
  save_coach.learn(model, x, y);
  std::cout << "first run: count = " << first_counter->count << "\n";

  ice::Outline resumed_model(nn::Fusion(2, 1));
  ice::TrainOptions resume_options = save_options;
  resume_options.execution.max_epochs = 3;
  resume_options.checkpoint.resume_from = "./ice_checkpoint_demo/demo-last.ckpt";

  const auto resumed_counter = std::make_shared<EpochCounterCallback>();
  std::cout << "fresh instance before resume: count = " << resumed_counter->count << "\n";

  ice::Coach resume_coach(resume_options);
  resume_coach.add_callback(resumed_counter);
  resume_coach.learn(resumed_model, x, y);
  std::cout << "after resumed run: count = " << resumed_counter->count << "\n";

  return 0;
}
```

Output

```text
first run: count = 2
fresh instance before resume: count = 0
after resumed run: count = 3
```

## See Also

- [`save_state`](save_state.md) -- what produces the map this restores
  from.
- [`on_checkpoint_load`](on_checkpoint_load.md) -- fires right after
  this, once per resumed callback.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="on_after_backward.html">
    <span class="label">Next</span>
    <span class="title">on_after_backward</span>
  </a>
</div>
<!-- apiref:nav:end -->
