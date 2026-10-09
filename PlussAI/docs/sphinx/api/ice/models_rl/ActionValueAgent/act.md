<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2cfcb826376b -->
# `pluss::ai::ice::models::ActionValueAgent::act`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
[[nodiscard]] std::int64_t pluss::ai::ice::models::ActionValueAgent::act(
    const pluss::Series& observation, double epsilon = 0.0, std::uint64_t seed = 0) const;
```

## What It Does

Picks one discrete action for a single `observation` --
epsilon-greedy: with probability `epsilon`, a uniformly random action
(seeded by `seed`, so the "random" choice is itself reproducible --
see Example 3); otherwise the greedy action
[`predict`](predict.md) itself would score highest. Unlike
[`predict`](predict.md), `observation` may be either a flat
`[observation_size]` vector or a `[1, observation_size]` row -- both
are accepted and agree (see Example 2).

## Parameters

`const Series& observation`
: A single observation, as `[observation_size]` or
  `[1, observation_size]`.

`double epsilon`
: Exploration probability, in `[0, 1]`. `0.0` (the default) is
  purely greedy.

`std::uint64_t seed`
: Seeds both the explore-or-not coin flip and the random action
  itself, when exploring.

## Returns

`std::int64_t` -- an action index in `[0, action_count)`.

## Errors

- Throws if the model has not been trained (`ready() == false`).
- Throws if `epsilon` is outside `[0, 1]`.
- Throws if `observation`'s shape is neither `[observation_size]` nor
  `[1, observation_size]`.

## Practical Examples

Three examples: the greedy action (`epsilon = 0.0`) matching the
optimal action in a tiny two-state chain, a flat and a row-shaped
observation agreeing, and `epsilon = 1.0` giving a seed-reproducible
"random" action.

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
  model_options.hidden_size = 16;
  model_options.hidden_layers = 2;
  model_options.discount = 0.9;
  model_options.target_sync_every = 8;
  model_options.seed = 21;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 120;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(64), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto state_a = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {2}, f32);
  const auto state_b = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);

  // Greedy (epsilon = 0.0, the default) -- act() should pick the same
  // action predict() scores highest.
  std::cout << "act(state_a) = " << agent.act(state_a) << "\n";
  std::cout << "act(state_b) = " << agent.act(state_b) << "\n";

  return 0;
}
```

Output

```text
act(state_a) = 1
act(state_b) = 0
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
  model_options.hidden_size = 16;
  model_options.hidden_layers = 2;
  model_options.discount = 0.9;
  model_options.target_sync_every = 8;
  model_options.seed = 21;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 120;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(64), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  // Flat [observation_size] and [1, observation_size] are both accepted,
  // and agree (unlike predict(), which requires the 2D shape).
  const auto flat = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {2}, f32);
  const auto row = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {1, 2}, f32);
  std::cout << "act(flat) == act(row): " << (agent.act(flat) == agent.act(row)) << "\n";

  return 0;
}
```

Output

```text
act(flat) == act(row): 1
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
  using pluss::ScalarType;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto state_a = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {2}, f32);

  // epsilon = 1.0 always explores -- the same seed reproduces the same
  // "random" action every time, regardless of the Q-network's own values.
  const auto a1 = agent.act(state_a, /*epsilon=*/1.0, /*seed=*/7ULL);
  const auto a2 = agent.act(state_a, /*epsilon=*/1.0, /*seed=*/7ULL);
  std::cout << "act() with epsilon=1.0 is seed-reproducible: " << (a1 == a2) << "\n";

  return 0;
}
```

Output

```text
act() with epsilon=1.0 is seed-reproducible: 1
```

## See Also

- [`predict`](predict.md) -- the raw Q-values this picks the max of
  when not exploring.
- [`learn`](learn.md) -- trains the model this call reads.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
