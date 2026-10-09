<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8776346c3861 -->
# `pluss::ai::ice::models::ActionValueAgent::to`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
pluss::ai::ice::models::ActionValueAgent& pluss::ai::ice::models::ActionValueAgent::to(
    pluss::ai::ice::Platform platform);
```

## What It Does

Moves both the online and target Q-networks' parameters to
`platform` in place, mutating this agent and returning `*this` so the
call chains directly into the next one (see Example 2) -- the same
real, in-place move
[`AutoregressiveForecaster::to`](../../models_ts/AutoregressiveForecaster/to.md)/[`TemporalFusionTransformer::to`](../../models_ts/TemporalFusionTransformer/to.md)
perform, unlike [`TextJudge::to`](../../models_nlp/TextJudge/to.md)'s
weaker, save/load-only version. Calling it with the platform the
agent is already on is a well-defined no-op -- predictions afterward
are byte-identical (see Example 1). [`ready()`](ready.md) is
unaffected either way.

## Parameters

`Platform platform`
: The target platform (`pluss::pCPU`, or a GPU platform if this build
  and machine support one).

## Returns

`ActionValueAgent&` -- `*this`, for chaining.

## Errors

None directly -- whatever moving a parameter's underlying `Series` to
`platform` would throw.

## Practical Examples

Three examples: `to(pCPU)` on an already-CPU agent leaving
predictions unchanged, the returned reference chaining straight into
`predict`, and `ready()` surviving the call.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

namespace {
pluss::ai::ice::models::ActionValueBatch make_chain_batch(std::int64_t repeats) {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  std::vector<float> obs{};
  std::vector<float> next_obs{};
  std::vector<std::int64_t> action{};
  std::vector<float> reward{};
  std::vector<float> done{};
  for (std::int64_t i = 0; i < repeats; ++i) {
    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 1.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(0.0F);

    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(0.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(1.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(1.0F);
  }

  const auto rows = static_cast<std::int64_t>(action.size());
  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto i64 = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  return models::ActionValueBatch{
      .observation = pluss::make_series(obs, {rows, 2}, f32),
      .action = pluss::make_series(action, {rows}, i64),
      .reward = pluss::make_series(reward, {rows}, f32),
      .next_observation = pluss::make_series(next_obs, {rows, 2}, f32),
      .done = pluss::make_series(done, {rows}, f32),
  };
}
} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 8;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto stacked = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  const auto before = agent.predict(stacked);
  agent.to(pluss::pCPU);
  const auto after = agent.predict(stacked);
  const double diff = before.sub(after).abs().amax().item<double>();
  std::cout << "to(pCPU) on an already-CPU agent is a no-op, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
to(pCPU) on an already-CPU agent is a no-op, diff = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

namespace {
pluss::ai::ice::models::ActionValueBatch make_chain_batch(std::int64_t repeats) {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  std::vector<float> obs{};
  std::vector<float> next_obs{};
  std::vector<std::int64_t> action{};
  std::vector<float> reward{};
  std::vector<float> done{};
  for (std::int64_t i = 0; i < repeats; ++i) {
    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 1.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(0.0F);

    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(0.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(1.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(1.0F);
  }

  const auto rows = static_cast<std::int64_t>(action.size());
  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto i64 = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  return models::ActionValueBatch{
      .observation = pluss::make_series(obs, {rows, 2}, f32),
      .action = pluss::make_series(action, {rows}, i64),
      .reward = pluss::make_series(reward, {rows}, f32),
      .next_observation = pluss::make_series(next_obs, {rows, 2}, f32),
      .done = pluss::make_series(done, {rows}, f32),
  };
}
} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 8;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto stacked = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  // to() returns *this by reference, so it chains directly into the next call.
  const auto q = agent.to(pluss::pCPU).predict(stacked);
  std::cout << "chained to().predict() sizes = [" << q.sizes()[0] << ", " << q.sizes()[1] << "]\n";

  return 0;
}
```

Output

```text
chained to().predict() sizes = [2, 2]
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

namespace {
pluss::ai::ice::models::ActionValueBatch make_chain_batch(std::int64_t repeats) {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  std::vector<float> obs{};
  std::vector<float> next_obs{};
  std::vector<std::int64_t> action{};
  std::vector<float> reward{};
  std::vector<float> done{};
  for (std::int64_t i = 0; i < repeats; ++i) {
    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 1.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(0.0F);

    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(0.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(1.0F);
    done.push_back(1.0F);

    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(1.0F);
  }

  const auto rows = static_cast<std::int64_t>(action.size());
  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto i64 = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  return models::ActionValueBatch{
      .observation = pluss::make_series(obs, {rows, 2}, f32),
      .action = pluss::make_series(action, {rows}, i64),
      .reward = pluss::make_series(reward, {rows}, f32),
      .next_observation = pluss::make_series(next_obs, {rows, 2}, f32),
      .done = pluss::make_series(done, {rows}, f32),
  };
}
} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 8;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);

  agent.to(pluss::pCPU);
  std::cout << "ready() survives to(), still = " << agent.ready() << "\n";

  return 0;
}
```

Output

```text
ready() survives to(), still = 1
```

## See Also

- [`ready`](ready.md) -- unaffected by `to`.
- [`predict`](predict.md) -- the usual call chained right after `to`
  when moving a loaded agent to a different platform for inference.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
