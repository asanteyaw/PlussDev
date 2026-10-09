# `pluss::ai::ice::models::ActionValueBatch`

One batch of transitions (observation, action, reward,
next_observation, done) for
[`ActionValueAgent::learn`/`assess`](ActionValueAgent.md).

## Signature

```cpp
pluss::ai::ice::models::ActionValueBatch::ActionValueBatch() = default;
```

A plain aggregate -- every field is `pluss::series::undefined()` by
default. Unlike every `models_ts`/`models_nlp` catalog entry's own
batch type, there is no converting constructor from a
[`data::Batch`](../data/Batch.md) -- `ActionValueAgent` has no
`data::`-family integration.

## State

### `observation`

`Series observation = pluss::series::undefined()`
: `[N, observation_size]`.

### `action`

`Series action = pluss::series::undefined()`
: `[N]`, `Int64` -- the discrete action index taken in `observation`.

### `reward`

`Series reward = pluss::series::undefined()`
: `[N]` -- the reward received after taking `action`.

### `next_observation`

`Series next_observation = pluss::series::undefined()`
: `[N, observation_size]` -- the state transitioned to.

### `done`

`Series done = pluss::series::undefined()`
: `[N]` -- `1.0` where the episode ended at this transition (masks
  out the bootstrap term in the TD target), `0.0` otherwise.

## Methods

None -- `ActionValueBatch` is a plain batch value; see `## State`
above.

## Practical Examples

Three examples: building one directly from raw transition tensors,
feeding one through a full `learn`/`assess` round trip, and a
mismatched `reward` shape being rejected.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto i64 = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);

  models::ActionValueBatch batch{};
  batch.observation = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  batch.action = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2}, i64);
  batch.reward = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);
  batch.next_observation = pluss::make_series(std::vector<float>{0.0f, 1.0f, 0.0f, 0.0f}, {2, 2}, f32);
  batch.done = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);

  std::cout << "observation.sizes() = [" << batch.observation.sizes()[0] << ", " << batch.observation.sizes()[1]
            << "]\n";
  std::cout << "action.defined() = " << batch.action.defined() << "\n";

  return 0;
}
```

Output

```text
observation.sizes() = [2, 2]
action.defined() = 1
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
  train_options.execution.max_epochs = 60;
  train_options.validation.split_ratio = 0.0;

  const auto batch = make_chain_batch(64);
  agent.learn(batch, train_options);

  const auto report = agent.assess(batch);
  std::cout << "ready() = " << agent.ready() << "\n";
  std::cout << "report.count = " << report.count << "\n";

  return 0;
}
```

Output

```text
ready() = 1
report.count = 256
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;
  using pluss::ScalarType;

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto i64 = pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;

  models::ActionValueAgent agent(model_options);

  models::ActionValueBatch mismatched{};
  mismatched.observation = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  mismatched.action = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2}, i64);
  // reward has only 1 entry for 2 rows -- a shape mismatch.
  mismatched.reward = pluss::make_series(std::vector<float>{0.0f}, {1}, f32);
  mismatched.next_observation = pluss::make_series(std::vector<float>{0.0f, 1.0f, 0.0f, 0.0f}, {2, 2}, f32);
  mismatched.done = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);

  bool threw = false;
  try {
    agent.learn(mismatched);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "learn() with a mismatched reward shape throws = " << threw << "\n";

  return 0;
}
```

Output

```text
learn() with a mismatched reward shape throws = 1
```

## Changelog

- **0.1** -- `ActionValueBatch`: `observation`/`action`/`reward`/
  `next_observation`/`done`.

## See Also

- [`ActionValueAgent::learn`/`assess`](ActionValueAgent.md) -- every
  consumer.
- [`ActionValueReport`](ActionValueReport.md) -- what `assess`
  returns.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
