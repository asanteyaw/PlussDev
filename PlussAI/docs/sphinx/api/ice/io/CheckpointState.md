# `pluss::ai::ice::io::CheckpointState`

The recipe, epoch/step counters, best-metric bookkeeping, RNG state,
and per-callback state map a full training-resumable checkpoint
carries -- built by the caller for
[`save_checkpoint`](save_checkpoint.md), returned by
[`load_checkpoint`](load_checkpoint.md). `Coach::learn` builds and
reads one internally on every checkpoint write/resume (see
[`on_checkpoint_save`](../callback/Callback/on_checkpoint_save.md)/
[`on_checkpoint_load`](../callback/Callback/on_checkpoint_load.md));
constructing one by hand is only necessary when calling
[`save_checkpoint`](save_checkpoint.md)/
[`load_checkpoint`](load_checkpoint.md) directly, outside `Coach`.

## Signature

```cpp
pluss::ai::ice::io::CheckpointState::CheckpointState() = default;
```

A plain aggregate -- every field defaults to a harmless empty/sentinel
value.

## State

### `recipe`

`TrainOptions recipe = {}`
: The [`TrainOptions`](../options/TrainOptions.md) the run was/will be
  driven by -- every field round-trips through the artifact (see
  [`save_checkpoint`](save_checkpoint.md) for the full list).

### `next_epoch`

`std::int64_t next_epoch = 0`
: The epoch a resumed run should start from. `Coach::learn` writes
  `epoch + 1` here on every save and reads it back as its own starting
  epoch on resume.

### `global_step`

`std::int64_t global_step = 0`
: The cumulative optimizer-step counter, carried across the resume
  boundary -- see [`RunState::global_step`](../callback/RunState.md).

### `best_metric` / `best_metric_name`

`std::optional<double> best_metric = std::nullopt`, `std::string best_metric_name = {}`
: The best value seen so far for the monitored metric, and that
  metric's own name -- mirrors
  [`RunState::best_metric`/`best_metric_name`](../callback/RunState.md).

### `best_checkpoint_path` / `last_checkpoint_path`

`std::string best_checkpoint_path = {}`, `std::string last_checkpoint_path = {}`
: The paths of the best-so-far and most-recent checkpoint writes --
  mirrors [`RunState`](../callback/RunState.md)'s own fields of the
  same name.

### `stop_reason`

`std::string stop_reason = {}`
: Why the run that wrote this checkpoint stopped, if it did --
  `"early_stopping"`, `"exception"`, or whatever a custom
  [`Callback`](../callback/Callback.md) wrote to
  [`RunState::stop_reason`](../callback/RunState.md).

### `rng_state`

`std::string rng_state = {}`
: A serialized snapshot of the training loop's own RNG, so a resumed
  run's shuffle order continues rather than restarting.

### `callback_state`

`std::unordered_map<std::string, std::unordered_map<std::string, std::string>> callback_state = {}`
: Every callback's own [`save_state`](../callback/Callback/save_state.md)
  result, keyed by its [`state_key`](../callback/Callback/state_key.md).
  [`load_checkpoint`](load_checkpoint.md) populates this but does
  **not** call [`load_state`](../callback/Callback/load_state.md) for
  you -- see that page's own Example 3 for why, and the manual
  restoration pattern to use instead.

## Methods

None -- `CheckpointState` is a plain read/write data struct; every
field is public, see `## State` above.

## Practical Examples

One example: a hand-built `CheckpointState` round-tripped through
[`save_checkpoint`](save_checkpoint.md)/[`load_checkpoint`](load_checkpoint.md)
directly, outside `Coach`.

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
```

## Changelog

- **0.1** -- `CheckpointState`: the full resumable-checkpoint payload,
  independent of `Model`/`Optimizer` weights (saved/loaded separately).

## See Also

- [`save_checkpoint`](save_checkpoint.md) / [`load_checkpoint`](load_checkpoint.md)
  -- the writer/reader this is the payload for.
- [`RunState`](../callback/RunState.md) -- the run-time counterpart
  several fields here mirror.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
