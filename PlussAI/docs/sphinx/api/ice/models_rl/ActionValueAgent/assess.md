<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37967139890b -->
# `pluss::ai::ice::models::ActionValueAgent::assess`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::models::ActionValueReport pluss::ai::ice::models::ActionValueAgent::assess(
    const ActionValueBatch& batch, bool use_huber = true) const;
```

## What It Does

Scores `batch`'s transitions against the same TD-error objective
[`learn`](learn.md) minimizes, returning an
[`ActionValueReport`](../ActionValueReport.md) -- mean loss, mean
Q-value, and transition count. `use_huber` picks which loss `loss`
reports (matching
[`ActionValueTrainOptions::use_huber`](../ActionValueTrainOptions.md)'s
own default); `mean_q` is unaffected either way, since it reads the
Q-network's own predictions, not the loss (see Example 2). `loss`
genuinely reflects fit quality -- scoring the same observations/
actions against unrelated reward noise scores measurably worse (see
Example 3).

## Parameters

`const ActionValueBatch& batch`
: The transitions to score.

`bool use_huber`
: Which TD-error loss `loss` reports -- `true` (the default) for
  Huber, `false` for MSE.

## Returns

[`ActionValueReport`](../ActionValueReport.md).

## Errors

- Throws if the model has not been trained (`ready() == false`).
- Throws if `batch`'s tensor shapes disagree with
  [`ActionValueAgentOptions`](../ActionValueAgentOptions.md), same as
  [`learn`](learn.md).

## Practical Examples

Three examples: a finite loss/mean-Q on the data the agent was
trained on, `use_huber` changing `loss` but never `mean_q`, and
`loss` rising on a batch whose rewards were swapped for unrelated
noise.

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
  const auto batch = make_chain_batch(64);
  agent.learn(batch, train_options);

  const auto good_report = agent.assess(batch);

  // Same observations/actions, but rewards replaced by unrelated noise --
  // a well-fit agent should score that noticeably worse.
  models::ActionValueBatch mismatched = batch;
  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  mismatched.reward = pluss::series::randn(mismatched.reward.sizes(), f32, 55ULL).mul(10.0);
  const auto bad_report = agent.assess(mismatched);

  std::cout << "good_report.loss < bad_report.loss: " << (good_report.loss < bad_report.loss) << "\n";

  return 0;
}
```

Output

```text
good_report.loss < bad_report.loss: 1
```

## See Also

- [`ActionValueReport`](../ActionValueReport.md) -- the returned
  type.
- [`learn`](learn.md) -- minimizes the same objective this reports.
- [`predict`](predict.md) / [`act`](act.md) -- per-observation Q-values
  and action choice, instead of an aggregate report.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="act.html">
    <span class="label">Previous</span>
    <span class="title">act</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
