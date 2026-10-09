<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0d31fd31b8ff -->
# `pluss::ai::ice::models::ActionValueAgent::ready`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
[[nodiscard]] bool pluss::ai::ice::models::ActionValueAgent::ready() const noexcept;
```

## What It Does

Reports whether [`learn`](learn.md) has run at least once (directly,
or indirectly via [`load`](load.md)). `false` for a freshly
constructed agent; `true` from then on, forever -- there is no way to
reset it short of constructing a new agent. [`predict`](predict.md)/
[`act`](act.md)/[`assess`](assess.md)/[`save`](save.md) all require
it to be `true`.

## Parameters

None.

## Returns

`bool`.

## Errors

None -- `noexcept`.

## Practical Examples

Three examples: the `false` -> `true` transition across `learn`,
`ready()` staying `true` across any number of subsequent `predict`/
`act`/`assess` calls, and a freshly `load`-ed agent already reporting
`true` with no `learn` call at all.

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

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 8;

  models::ActionValueAgent agent(model_options);
  std::cout << "ready() before learn() = " << agent.ready() << "\n";

  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);
  std::cout << "ready() after learn() = " << agent.ready() << "\n";

  return 0;
}
```

Output

```text
ready() before learn() = 0
ready() after learn() = 1
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
  const auto batch = make_chain_batch(8);
  agent.learn(batch, train_options);

  // ready() stays true across repeated predict()/act()/assess() calls.
  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto state_a = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {1, 2}, f32);
  const auto q = agent.predict(state_a);
  const auto a = agent.act(state_a.reshape({2}));
  const auto report = agent.assess(batch);
  static_cast<void>(q);
  static_cast<void>(a);
  static_cast<void>(report);
  std::cout << "ready() after predict()/act()/assess() = " << agent.ready() << "\n";

  return 0;
}
```

Output

```text
ready() after predict()/act()/assess() = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <filesystem>
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

  const auto path = std::filesystem::temp_directory_path() / "ice_ava_ready_load.artifact";
  agent.save(path.string());

  const auto loaded = models::ActionValueAgent::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".online.weights");
  std::filesystem::remove(path.string() + ".target.weights");

  // ready() is true immediately -- load() never requires a follow-up
  // learn() call.
  std::cout << "loaded.ready() without calling learn() = " << loaded.ready() << "\n";

  return 0;
}
```

Output

```text
loaded.ready() without calling learn() = 1
```

## See Also

- [`learn`](learn.md) -- the only way to make a freshly constructed
  agent `ready()`.
- [`load`](load.md) -- the other way, skipping `learn` entirely.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="predict.html">
    <span class="label">Previous</span>
    <span class="title">predict</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
