<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bd153763d820 -->
# `pluss::ai::ice::Callback::state_key`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
[[nodiscard]] virtual std::string pluss::ai::ice::Callback::state_key() const;
```

## What It Does

Names the slot this callback's own state is stored under inside a full
(non-weights-only) checkpoint. The default implementation returns an
empty string, which means "this callback carries no state worth
persisting" -- `Coach::learn`'s checkpoint logic skips calling
[`save_state`](save_state.md)/[`load_state`](load_state.md) entirely
for any callback whose `state_key()` is empty. Override this (returning
a fixed, non-empty, unique-among-your-callbacks name) alongside
`save_state`/`load_state` for any callback whose behavior depends on
state that should survive a checkpoint resume.
[`EarlyStopCallback`](../EarlyStopCallback.md) returns `"early_stop"`.

## Parameters

None.

## Returns

`std::string` -- empty by default; a non-empty key opts into
checkpoint persistence.

## Errors

None.

## Practical Examples

Three examples: the default empty key, a custom override, and the key
that actually triggers a real checkpoint save/resume round trip for a
callback's own state.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

namespace {

class PlainCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  const PlainCallback callback;
  std::cout << "state_key() == \"\": " << (callback.state_key().empty()) << "\n";

  return 0;
}
```

Output

```text
state_key() == "": 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>

namespace {

class EpochCounterCallback final : public pluss::ai::ice::Callback {
 public:
  std::string state_key() const override { return "epoch_counter"; }
};

} // namespace

int main() {
  const EpochCounterCallback callback;
  std::cout << "state_key() = \"" << callback.state_key() << "\"\n";

  return 0;
}
```

Output

```text
state_key() = "epoch_counter"
```

### Example 3

A callback that counts epochs, with `state_key`/`save_state`/
`load_state` all overridden, survives a checkpoint resume -- a *fresh*
instance added to the resuming `Coach` picks up exactly where the first
run's instance left off, because `state_key()` gave `Coach` a name to
store and retrieve its state under.

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

- [`save_state`](save_state.md) / [`load_state`](load_state.md) -- what
  gets stored under and retrieved by this key.
- [`EarlyStopCallback`](../EarlyStopCallback.md) -- the built-in
  callback that uses `"early_stop"`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_state.html">
    <span class="label">Previous</span>
    <span class="title">save_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
