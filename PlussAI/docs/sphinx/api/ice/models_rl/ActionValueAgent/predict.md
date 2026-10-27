<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b7efe50e947 -->
# `pluss::ai::ice::models::ActionValueAgent::predict`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
[[nodiscard]] pluss::Series pluss::ai::ice::models::ActionValueAgent::predict(
    const pluss::Series& observations) const;
```

## What It Does

Runs the online Q-network forward in evaluation mode and returns raw
Q-values for every action, at every row of `observations` -- unlike
every other catalog entry's `predict`, there is no dedicated result
type here; the returned `Series` *is* the answer. The highest-scoring
column at each row is the greedy action (see Example 1) -- exactly
what [`act`](act.md) picks with `epsilon = 0`.

## Parameters

`const Series& observations`
: `[N, observation_size]` -- always two-dimensional, even for a
  single observation (reshape a flat `[observation_size]` vector to
  `[1, observation_size]` first, or call [`act`](act.md) instead,
  which accepts either shape -- see Example 2).

## Returns

`Series`, shaped `[N, action_count]`. Deterministic for a fixed
trained agent and a fixed input (see Example 3).

## Errors

- Throws if the model has not been trained (`ready() == false`).
- Throws if `observations` is not two-dimensional with its second
  axis equal to
  [`ActionValueAgentOptions::observation_size`](../ActionValueAgentOptions.md).

## Practical Examples

Three examples: the greedy action scoring highest at each row for a
trained agent, a flat 1D observation being rejected (unlike
[`act`](act.md), which accepts one), and repeated calls on the same
input being bit-for-bit identical.

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
  // state A = [1,0] (good action is 1), state B = [0,1] (good action is 0).
  const auto stacked = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  const auto q = agent.predict(stacked).to(pluss::pCPU, ScalarType::Float64);
  const auto qv = q.reshape({4}).pull<double>();

  std::cout << "q.sizes() = [" << q.reshape({2, 2}).sizes()[0] << ", " << q.reshape({2, 2}).sizes()[1] << "]\n";
  std::cout << "state A scores action 1 highest: " << (qv[1] > qv[0]) << "\n";
  std::cout << "state B scores action 0 highest: " << (qv[2] > qv[3]) << "\n";

  return 0;
}
```

Output

```text
q.sizes() = [2, 2]
state A scores action 1 highest: 1
state B scores action 0 highest: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
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
  // predict() requires a 2D [N, observation_size] batch -- a flat
  // [observation_size] vector (as act() itself accepts) is rejected here.
  const auto flat_observation = pluss::make_series(std::vector<float>{1.0f, 0.0f}, {2}, f32);
  bool threw = false;
  try {
    const auto q = agent.predict(flat_observation);
    static_cast<void>(q);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "predict() with a flat 1D observation throws = " << threw << "\n";

  return 0;
}
```

Output

```text
predict() with a flat 1D observation throws = 1
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
  train_options.execution.max_epochs = 20;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(32), train_options);

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto stacked = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  const auto first = agent.predict(stacked);
  const auto second = agent.predict(stacked);
  const double diff = first.sub(second).abs().amax().item<double>();
  std::cout << "repeated predict() on a trained agent is deterministic, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
repeated predict() on a trained agent is deterministic, diff = 0
```

## See Also

- [`act`](act.md) -- picks the greedy action from this same Q-network,
  with optional epsilon-greedy exploration.
- [`learn`](learn.md) -- trains the model this call reads.
- [`assess`](assess.md) -- a loss/mean-Q summary for a full batch of
  transitions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
