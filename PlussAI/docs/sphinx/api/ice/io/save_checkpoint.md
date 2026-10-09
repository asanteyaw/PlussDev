# `pluss::ai::ice::io::save_checkpoint`

Writes a full training-resumable checkpoint: model parameters,
optimizer state, [`CheckpointState`](CheckpointState.md), and every
callback's own saved state. What
[`Coach::learn`](../coach/Coach/learn.md) calls internally on every
[`CheckpointPolicy::save_best`/`save_last`/`save_on_exception`](../options/CheckpointPolicy.md)
write (see
[`on_checkpoint_save`](../callback/Callback/on_checkpoint_save.md));
exposed standalone for checkpointing a hand-rolled training loop built
on [`make_optimizer`](../optim/make_optimizer.md), with the same
artifact format `Coach`'s own checkpoints use.

## Signature

```cpp
void pluss::ai::ice::io::save_checkpoint(
    std::string_view filepath,
    const pluss::ai::ice::Model& model,
    const pluss::ai::nn::optim::Optimizer& optimizer,
    const pluss::ai::ice::io::CheckpointState& state,
    const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks = {});
```

## What It Does

Calls `model.save(filepath + ".weights")` for the parameters, captures
`optimizer`'s own internal state via its concrete type (one of `Adam`/
`AdamW`/`SGD`/`RMSprop`/`Adagrad`/`NAdam` -- any other concrete
`Optimizer` type throws), writes every field of `state` and
`TrainOptions::recipe`, and -- for each callback in `callbacks` whose
[`state_key`](../callback/Callback/state_key.md) is non-empty -- writes
that callback's own [`save_state`](../callback/Callback/save_state.md)
result too, all into one checksummed file at `filepath`.

## Parameters

`std::string_view filepath`
: Where to write. The model weights land in a sibling file,
  `filepath + ".weights"`; the main file records that path so
  [`load_checkpoint`](load_checkpoint.md) finds it automatically.

`const Model& model`
: Saved via its own public `save` -- same format
  [`Model::save`](../model/Model/save.md) produces standalone.

`const Optimizer& optimizer`
: Its concrete type is detected by `dynamic_cast`; its own
  `capture_state()` is what gets serialized.

`const CheckpointState& state`
: Everything beyond parameters/optimizer state -- see
  [`CheckpointState`](CheckpointState.md) field by field.

`const std::vector<std::shared_ptr<Callback>>& callbacks`
: Each callback whose `state_key()` is non-empty gets its own
  `save_state()` written too, under that key. Empty by default --
  pass `{}` (or omit) when no callback carries state worth persisting.

## Returns

`void`.

## Errors

- Throws if `filepath` is empty.
- Throws if `optimizer`'s concrete type is not one of the six
  supported kinds.
- Throws whatever `Model::save`/the output file stream throws.

## Practical Examples

Three examples: a direct save against a trained model and a standalone
optimizer, the full round trip through
[`load_checkpoint`](load_checkpoint.md) confirming every
`CheckpointState` field and the model's weights, and a callback's own
state being written and later readable from the restored
`CheckpointState::callback_state` map.

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

  // The optimizer's own state is captured/restored independently of the
  // model's weights -- any live Optimizer instance works here, including
  // one built standalone via make_optimizer over its own parameters.
  Series w = pluss::make_series({0.0f}, {1});
  const auto optimizer = ice::make_optimizer({&w}, options);

  ice::io::CheckpointState state{};
  state.recipe = options;
  state.next_epoch = 50;
  state.global_step = 200;
  state.best_metric = 0.05;
  state.best_metric_name = "rmse";

  ice::io::save_checkpoint("./io_checkpoint_demo.ckpt", model, *optimizer, state);
  std::cout << "saved ok\n";

  return 0;
}
```

Output

```text
saved ok
```

### Example 2

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

### Example 3

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

## See Also

- [`load_checkpoint`](load_checkpoint.md) -- the matching reader.
- [`CheckpointState`](CheckpointState.md) -- the `state` parameter,
  field by field.
- [`on_checkpoint_save`](../callback/Callback/on_checkpoint_save.md) --
  fires right after `Coach::learn`'s own internal call to this.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
