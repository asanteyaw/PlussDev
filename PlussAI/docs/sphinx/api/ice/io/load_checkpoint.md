# `pluss::ai::ice::io::load_checkpoint`

Restores a full training-resumable checkpoint into an
already-constructed model/optimizer/callback set, returning the
[`CheckpointState`](CheckpointState.md) to resume the training loop
from. What [`Coach::learn`](../coach/Coach/learn.md) calls internally
when [`CheckpointPolicy::resume_from`](../options/CheckpointPolicy.md)
is set (see
[`on_checkpoint_load`](../callback/Callback/on_checkpoint_load.md)).

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::io::CheckpointState pluss::ai::ice::io::load_checkpoint(
    std::string_view filepath,
    pluss::ai::ice::Model& model,
    pluss::ai::nn::optim::Optimizer& optimizer,
    const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks = {});
```

## What It Does

Verifies the checksum, calls `model.load(weights_path)` using the
sidecar path [`save_checkpoint`](save_checkpoint.md) recorded, restores
`optimizer`'s own state (throws if its concrete type does not match
what was saved), and returns a fully populated
[`CheckpointState`](CheckpointState.md) -- including
`callback_state`, every saved callback's own state keyed by its
[`state_key`](../callback/Callback/state_key.md).

`callbacks` is accepted for signature symmetry with
[`save_checkpoint`](save_checkpoint.md), but is **not** used to call
[`load_state`](../callback/Callback/load_state.md) for you -- see
Example 3. A stateful callback like
[`EarlyStopCallback`](../callback/EarlyStopCallback.md) unconditionally
resets its own state in
[`on_run_begin`](../callback/Callback/on_run_begin.md), which
`Coach::learn` fires *after* calling this function; restoring state
here (before `on_run_begin` even runs) would just be overwritten a
moment later. `Coach::learn` applies `callback_state` itself, right
after its own `on_run_begin` -- see Example 3 for that exact pattern.

## Parameters

`std::string_view filepath`
: The checkpoint [`save_checkpoint`](save_checkpoint.md) wrote.

`Model& model`
: Restored in place via its own `load`.

`Optimizer& optimizer`
: Restored in place -- must already be the same concrete type the
  checkpoint was saved from.

`const std::vector<std::shared_ptr<Callback>>& callbacks`
: Present for symmetry with [`save_checkpoint`](save_checkpoint.md);
  not read. Apply `CheckpointState::callback_state` to your own
  callbacks by hand, at the point in your own loop equivalent to
  `Coach::learn`'s own restoration (after whatever hook resets their
  state, if any).

## Returns

[`CheckpointState`](CheckpointState.md) -- everything beyond
`model`/`optimizer`, including `next_epoch` (where a resumed loop
should start) and `callback_state`.

## Errors

- Throws if `filepath` is empty, cannot be opened, or fails the
  checksum.
- Throws if the checkpoint's schema is unsupported.
- Throws if `optimizer`'s concrete type does not match what was saved.
- Throws whatever `Model::load` throws for a mismatched architecture.

## Practical Examples

Three examples: a direct restore confirming every `CheckpointState`
field and the model's weights, reading a saved callback's state back
out of `CheckpointState::callback_state`, and the manual restoration
pattern -- applying that state to a fresh callback instance, the exact
shape `Coach::learn` itself uses.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::Series;

  ice::Outline model(nn::Fusion(2, 1));
  const Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 50;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  Series w = pluss::make_series({0.0f}, {1});
  const auto optimizer = ice::make_optimizer({&w}, options);

  ice::io::CheckpointState state{};
  state.recipe = options;
  state.next_epoch = 50;
  state.global_step = 200;
  state.best_metric = 0.05;
  state.best_metric_name = "rmse";
  ice::io::save_checkpoint("./io_checkpoint_demo.ckpt", model, *optimizer, state);

  ice::Outline resumed_model(nn::Fusion(2, 1));
  Series w2 = pluss::make_series({0.0f}, {1});
  const auto resumed_optimizer = ice::make_optimizer({&w2}, options);

  const ice::io::CheckpointState restored =
      ice::io::load_checkpoint("./io_checkpoint_demo.ckpt", resumed_model, *resumed_optimizer);

  std::cout << "restored.next_epoch = " << restored.next_epoch << "\n";
  std::cout << "restored.global_step = " << restored.global_step << "\n";
  std::cout << "restored.best_metric = " << restored.best_metric.value_or(-1.0) << "\n";
  std::cout << "restored.best_metric_name = " << restored.best_metric_name << "\n";
  std::cout << "restored.recipe.learning_rate = " << restored.recipe.learning_rate << "\n";

  const Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});
  const double diff = model.infer(one_row).sub(resumed_model.infer(one_row)).abs().amax().item<double>();
  std::cout << "model weights match: " << (diff == 0.0) << "\n";

  return 0;
}
```

Output

```text
restored.next_epoch = 50
restored.global_step = 200
restored.best_metric = 0.05
restored.best_metric_name = rmse
restored.recipe.learning_rate = 0.1
model weights match: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

class CounterCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 7;
  std::string state_key() const override { return "counter"; }
  std::unordered_map<std::string, std::string> save_state() const override {
    return {{"count", std::to_string(count)}};
  }
  void load_state(const std::unordered_map<std::string, std::string>& state) override {
    const auto it = state.find("count");
    if (it != state.end()) count = std::stoi(it->second);
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::Series;

  ice::Outline model(nn::Fusion(2, 1));
  const Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  Series w = pluss::make_series({0.0f}, {1});
  const auto optimizer = ice::make_optimizer({&w}, options);

  const auto counter = std::make_shared<CounterCallback>();
  ice::io::CheckpointState state{};
  ice::io::save_checkpoint("./io_checkpoint_cb_demo.ckpt", model, *optimizer, state, {counter});

  ice::Outline resumed_model(nn::Fusion(2, 1));
  Series w2 = pluss::make_series({0.0f}, {1});
  const auto resumed_optimizer = ice::make_optimizer({&w2}, options);
  const ice::io::CheckpointState restored =
      ice::io::load_checkpoint("./io_checkpoint_cb_demo.ckpt", resumed_model, *resumed_optimizer);

  std::cout << "restored.callback_state has \"counter\" key: "
             << (restored.callback_state.find("counter") != restored.callback_state.end()) << "\n";
  std::cout << "restored.callback_state[\"counter\"][\"count\"] = "
             << restored.callback_state.at("counter").at("count") << "\n";

  return 0;
}
```

Output

```text
restored.callback_state has "counter" key: 1
restored.callback_state["counter"]["count"] = 7
```

### Example 3

The `callbacks` parameter above was never used to restore `counter`'s
state -- it stayed at its default `7` all along, never changed. This
example applies `CheckpointState::callback_state` to a *fresh*
instance by hand, the exact pattern `Coach::learn` itself runs (after
its own `on_run_begin`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

class CounterCallback final : public pluss::ai::ice::Callback {
 public:
  int count = 7;
  std::string state_key() const override { return "counter"; }
  std::unordered_map<std::string, std::string> save_state() const override {
    return {{"count", std::to_string(count)}};
  }
  void load_state(const std::unordered_map<std::string, std::string>& state) override {
    const auto it = state.find("count");
    if (it != state.end()) count = std::stoi(it->second);
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::Series;

  ice::Outline model(nn::Fusion(2, 1));
  const Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});
  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  Series w = pluss::make_series({0.0f}, {1});
  const auto optimizer = ice::make_optimizer({&w}, options);

  const auto counter = std::make_shared<CounterCallback>();
  ice::io::CheckpointState state{};
  ice::io::save_checkpoint("./io_checkpoint_cb_demo.ckpt", model, *optimizer, state, {counter});

  ice::Outline resumed_model(nn::Fusion(2, 1));
  Series w2 = pluss::make_series({0.0f}, {1});
  const auto resumed_optimizer = ice::make_optimizer({&w2}, options);
  const ice::io::CheckpointState restored =
      ice::io::load_checkpoint("./io_checkpoint_cb_demo.ckpt", resumed_model, *resumed_optimizer);

  const auto fresh_counter = std::make_shared<CounterCallback>();
  fresh_counter->count = 0;
  const auto it = restored.callback_state.find(fresh_counter->state_key());
  if (it != restored.callback_state.end()) {
    fresh_counter->load_state(it->second);
  }
  std::cout << "fresh_counter->count after manual restore = " << fresh_counter->count << "\n";

  return 0;
}
```

Output

```text
fresh_counter->count after manual restore = 7
```

## See Also

- [`save_checkpoint`](save_checkpoint.md) -- the matching writer.
- [`CheckpointState`](CheckpointState.md) -- the full return type.
- [`Coach`](../coach/Coach.md) -- applies `callback_state` for you,
  automatically, as part of `learn`'s own resume handling.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
