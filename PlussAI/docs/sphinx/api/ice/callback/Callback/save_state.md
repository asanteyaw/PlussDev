<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c504d8b07f38 -->
# `pluss::ai::ice::Callback::save_state`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
[[nodiscard]] virtual std::unordered_map<std::string, std::string> pluss::ai::ice::Callback::save_state() const;
```

## What It Does

Returns this callback's own persistable state, as a flat string-keyed
map -- `Coach::learn` calls this on every callback whose
[`state_key`](state_key.md) is non-empty, right before every checkpoint
write, and stores the result under that key inside the checkpoint.
`EarlyStopCallback`'s own version returns its monitor/patience/best
value/etc as strings; see
[its own Introduction](../EarlyStopCallback/Introduction.md) for the
exact fields. The default implementation returns an empty map.

## Parameters

None.

## Returns

`std::unordered_map<std::string, std::string>` -- empty by default.
Every value is stored and read back as plain text, so numeric fields
need an explicit `std::to_string`/parse round trip in the override (see
Example 2).

## Errors

None.

## Practical Examples

Three examples: the default empty map, a custom override returning
real values, and this method actually called by `Coach::learn` on every
checkpoint write during a real training run.

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
  std::cout << "save_state().empty(): " << (callback.save_state().empty()) << "\n";

  return 0;
}
```

Output

```text
save_state().empty(): 1
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
  int count = 5;
  std::unordered_map<std::string, std::string> save_state() const override {
    return {{"count", std::to_string(count)}};
  }
};

} // namespace

int main() {
  const EpochCounterCallback callback;
  const auto state = callback.save_state();
  std::cout << "save_state()[\"count\"] = " << state.at("count") << "\n";

  return 0;
}
```

Output

```text
save_state()["count"] = 5
```

### Example 3

`Coach::learn` calls `save_state` itself, once per checkpoint write --
here the resulting value is what lets a resumed run's fresh callback
instance pick its count back up through
[`load_state`](load_state.md).

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

- [`state_key`](state_key.md) -- the key this value is stored under.
- [`load_state`](load_state.md) -- the restore side.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_run_end.html">
    <span class="label">Previous</span>
    <span class="title">on_run_end</span>
  </a>
  <a class="next-link" href="state_key.html">
    <span class="label">Next</span>
    <span class="title">state_key</span>
  </a>
</div>
<!-- apiref:nav:end -->
