<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 13095c277a54 -->
# `pluss::ai::ice::models::ActionValueAgent::learn`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
void pluss::ai::ice::models::ActionValueAgent::learn(
    const ActionValueBatch& train, const ActionValueTrainOptions& options = {});
void pluss::ai::ice::models::ActionValueAgent::learn(
    const ActionValueBatch& train, const ActionValueBatch& val,
    const ActionValueTrainOptions& options = {});
```

## What It Does

Trains a double-DQN-style agent in place on `train`'s transitions,
leaving it [`ready()`](ready.md) afterward -- re-initializing both
the online and target Q-networks from scratch each call, rather than
continuing from whatever the agent was previously trained to. Unlike
every `models_ts`/`models_nlp` catalog entry, there are only two
overloads here, and no `data::Batch`/`data::TimeSeriesBatch`
convenience overloads at all -- `ActionValueAgent` has no
`data::`-family integration. Passing `val` explicitly evaluates on
that exact held-out batch every epoch for
[`loop_regularization`](../ActionValueTrainOptions.md)/early
stopping; the single-batch overload instead carves a split out of
`train` itself per `options.validation.split_ratio`.

## Parameters

`const ActionValueBatch& train`
: The training transitions.

`const ActionValueBatch& val`
: An explicit held-out validation batch of transitions.

`const ActionValueTrainOptions& options`
: Batch size, learning rate, optimizer, the Huber-vs-MSE loss toggle,
  and the shared validation/regularization/execution policies -- see
  [`ActionValueTrainOptions`](../ActionValueTrainOptions.md) field by
  field.

## Returns

`void` -- mutates the model in place; [`ready()`](ready.md) becomes
`true` afterward.

## Errors

- Throws if `train`/`val`'s tensor shapes disagree with
  [`ActionValueAgentOptions`](../ActionValueAgentOptions.md)'s own
  `observation_size`/`action_count`, or if any of `observation`/
  `action`/`reward`/`next_observation`/`done` has a different leading
  dimension from the others.
- Throws per [`ExecutionPolicy::fail_on_non_finite`](../../options/ExecutionPolicy.md)
  if a non-finite loss/gradient appears during training (surfaced as
  a `learn`-specific "training became numerically unstable" error).

## Practical Examples

Three examples: the single-batch overload taking a model from
un-trained to `ready()`, the two-batch overload training against an
explicit validation batch with early stopping, and a batch whose
observation width disagrees with
[`ActionValueAgentOptions::observation_size`](../ActionValueAgentOptions.md)
being rejected.

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
  model_options.hidden_size = 16;
  model_options.hidden_layers = 2;
  model_options.discount = 0.9;
  model_options.target_sync_every = 8;
  model_options.seed = 21;

  models::ActionValueAgent agent(model_options);
  std::cout << "ready() before learn() = " << agent.ready() << "\n";

  models::ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 60;
  train_options.validation.split_ratio = 0.0;

  const auto batch = make_chain_batch(64);
  agent.learn(batch, train_options);
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
  const auto train = make_chain_batch(64);
  const auto val = make_chain_batch(16);

  models::ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 60;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 10;

  // Explicit held-out validation batch -- evaluated every epoch for early
  // stopping, instead of carving a split out of `train`.
  agent.learn(train, val, train_options);
  std::cout << "ready() = " << agent.ready() << "\n";

  const auto val_report = agent.assess(val);
  std::cout << "assess(val).loss is finite = " << std::isfinite(val_report.loss) << "\n";

  return 0;
}
```

Output

```text
ready() = 1
assess(val).loss is finite = 1
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

  // observation has width 3, but the model was configured for observation_size = 2.
  models::ActionValueBatch wrong_width{};
  wrong_width.observation = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}, {2, 3}, f32);
  wrong_width.action = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2}, i64);
  wrong_width.reward = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);
  wrong_width.next_observation =
      pluss::make_series(std::vector<float>{0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {2, 3}, f32);
  wrong_width.done = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);

  bool threw = false;
  try {
    agent.learn(wrong_width);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "learn() with the wrong observation width throws = " << threw << "\n";

  return 0;
}
```

Output

```text
learn() with the wrong observation width throws = 1
```

## See Also

- [`ActionValueBatch`](../ActionValueBatch.md) -- the transition
  batch type.
- [`ActionValueTrainOptions`](../ActionValueTrainOptions.md) -- every
  training knob.
- [`predict`](predict.md) / [`act`](act.md) / [`assess`](assess.md)
  -- the usual next calls on a trained agent.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
