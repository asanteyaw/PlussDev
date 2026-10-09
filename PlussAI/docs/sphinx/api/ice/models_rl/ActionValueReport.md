# `pluss::ai::ice::models::ActionValueReport`

The TD loss, mean Q-value, and transition count
[`ActionValueAgent::assess`](ActionValueAgent/assess.md) returns.

## Signature

```cpp
pluss::ai::ice::models::ActionValueReport::ActionValueReport() = default;
```

A plain aggregate -- `loss`/`mean_q`/`count` are all `0` by default.

## State

### `loss`

`double loss = 0.0`
: The mean TD error over the scored batch -- Huber or MSE, per
  `assess`'s own `use_huber` argument (see Example 3).

### `mean_q`

`double mean_q = 0.0`
: The mean predicted Q-value (for the taken action) over the scored
  batch -- the same regardless of `use_huber`, since the loss choice
  only changes how the TD error is penalized, not what the Q-network
  itself predicts.

### `count`

`std::int64_t count = 0`
: Number of transitions scored -- the leading dimension of the
  [`ActionValueBatch`](ActionValueBatch.md) passed to `assess`.

## Methods

None -- `ActionValueReport` is a plain read-only result value; see
`## State` above.

## Practical Examples

Three examples: `assess()`'s `count` matching the scored batch size
with finite `loss`/`mean_q`, building one by hand, and `use_huber`
changing `loss` but never `mean_q`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <cmath>
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
  std::cout << "report.count = " << report.count << "\n";
  std::cout << "report.loss is finite = " << std::isfinite(report.loss) << "\n";
  std::cout << "report.mean_q is finite = " << std::isfinite(report.mean_q) << "\n";

  return 0;
}
```

Output

```text
report.count = 256
report.loss is finite = 1
report.mean_q is finite = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::ActionValueReport report{};
  report.loss = 0.04;
  report.mean_q = 0.72;
  report.count = 256;

  std::cout << "loss = " << report.loss << "\n";
  std::cout << "mean_q = " << report.mean_q << "\n";
  std::cout << "count = " << report.count << "\n";

  return 0;
}
```

Output

```text
loss = 0.04
mean_q = 0.72
count = 256
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
  model_options.hidden_size = 16;
  model_options.hidden_layers = 2;
  model_options.discount = 0.9;
  model_options.target_sync_every = 8;
  model_options.seed = 21;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 3; // deliberately under-trained -- TD error stays nonzero.
  train_options.validation.split_ratio = 0.0;

  const auto batch = make_chain_batch(64);
  agent.learn(batch, train_options);

  // use_huber toggles which TD-error loss assess() reports.
  const auto huber_report = agent.assess(batch, /*use_huber=*/true);
  const auto mse_report = agent.assess(batch, /*use_huber=*/false);
  std::cout << "huber_report.loss != mse_report.loss: " << (huber_report.loss != mse_report.loss) << "\n";
  std::cout << "huber_report.mean_q == mse_report.mean_q: " << (huber_report.mean_q == mse_report.mean_q) << "\n";

  return 0;
}
```

Output

```text
huber_report.loss != mse_report.loss: 1
huber_report.mean_q == mse_report.mean_q: 1
```

## Changelog

- **0.1** -- `ActionValueReport`: `loss`/`mean_q`/`count`.

## See Also

- [`ActionValueAgent::assess`](ActionValueAgent/assess.md) -- the
  only producer.
- [`ActionValueBatch`](ActionValueBatch.md) -- the scored input.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
