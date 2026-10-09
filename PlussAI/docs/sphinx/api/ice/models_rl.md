# `pluss::ai::ice::models` (models_rl)

`models_rl` is ice's reinforcement-learning catalog: one entry,
[`ActionValueAgent`](models_rl/ActionValueAgent.md), a double-DQN-style
discrete action-value agent, plus its own `Options`/`TrainOptions`/
batch/report types.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`ActionValueAgentOptions`](models_rl/ActionValueAgentOptions.md) | observation/action sizing, hidden width/depth, discount, double-Q, target-sync cadence |
| [`ActionValueTrainOptions`](models_rl/ActionValueTrainOptions.md) | batch size/learning rate/optimizer/Huber-vs-MSE/shared policies for `ActionValueAgent::learn` |
| [`ActionValueBatch`](models_rl/ActionValueBatch.md) | one batch of transitions -- observation/action/reward/next_observation/done |
| [`ActionValueReport`](models_rl/ActionValueReport.md) | the TD loss/mean-Q/count `ActionValueAgent::assess` returns |
| [`ActionValueAgent`](models_rl/ActionValueAgent.md) | the agent itself: `learn`/`predict`/`act`/`assess`/`save`/`load`/`ready`/`to` |

## How To Call It

Train an action-value agent on a small transition batch, then choose an action with epsilon-greedy exploration. The program checks that the chosen action is a valid index.

Train an action-value agent on a small transition batch, then choose an action with epsilon-greedy exploration. The program checks that the chosen action is a valid index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  models::ActionValueAgentOptions options{};
  options.observation_size = 4;
  options.action_count = 2;
  models::ActionValueAgent agent(options);

  const std::int64_t n = 4;
  models::ActionValueBatch transitions{};
  transitions.observation = pluss::make_series(std::vector<double>(n * 4, 0.5), {n, 4}, cfg);
  transitions.action = pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0}, {n}, cfg);
  transitions.reward = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 0.0}, {n}, cfg);
  transitions.next_observation = pluss::make_series(std::vector<double>(n * 4, 0.25), {n, 4}, cfg);
  transitions.done = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0}, {n}, cfg);

  agent.learn(transitions);
  const pluss::Series observation = pluss::make_series(std::vector<double>{0.5, 0.5, 0.5, 0.5}, {4}, cfg);
  const std::int64_t action = agent.act(observation, /*epsilon=*/0.1);
  std::cout << "action in range = " << ((action == 0 || action == 1) ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
action in range = true
```

Output

```text
action in range = true
```

Unlike `models_ts`/`models_nlp`, there is no `data::`-family
integration here at all -- build
[`ActionValueBatch`](models_rl/ActionValueBatch.md) directly from
whatever tensors an environment/replay buffer produces.

## Related Pages

- [`models_ts`](models_ts.md) / [`models_nlp`](models_nlp.md) -- the
  other two `models_*` families.
- [`options`](options.md) -- the nested policy structs
  `ActionValueTrainOptions` shares with
  [`TrainOptions`](options/TrainOptions.md).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

models_rl/Introduction
models_rl/ActionValueAgentOptions
models_rl/ActionValueTrainOptions
models_rl/ActionValueBatch
models_rl/ActionValueReport
models_rl/ActionValueAgent
```
<!-- apiref:toctree:end -->
