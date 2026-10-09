# `pluss::ai::ice::models::ActionValueAgent`

A double-DQN-style discrete action-value agent: learn from
transitions, predict Q-values, or act with epsilon-greedy
exploration.

## Signature

```cpp
explicit pluss::ai::ice::models::ActionValueAgent::ActionValueAgent(
    pluss::ai::ice::models::ActionValueAgentOptions options);
```

## Construction

```cpp
namespace ice = pluss::ai::ice;
namespace models = ice::models;

models::ActionValueAgentOptions options{};
options.observation_size = 4;
options.action_count = 2;
options.hidden_size = 64;

models::ActionValueAgent agent(options);
```

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `act(observation, epsilon = 0.0, seed = 0)` | `std::int64_t` | [`act`](ActionValueAgent/act.md) |
| `assess(batch, use_huber = true)` | `ActionValueReport` | [`assess`](ActionValueAgent/assess.md) |
| `learn(train, options = {})` / `learn(train, val, options = {})` | `void` | [`learn`](ActionValueAgent/learn.md) |
| `load(filepath)` | `ActionValueAgent` | [`load`](ActionValueAgent/load.md) |
| `predict(observations)` | `Series` | [`predict`](ActionValueAgent/predict.md) |
| `ready()` | `bool` | [`ready`](ActionValueAgent/ready.md) |
| `save(filepath)` | `void` | [`save`](ActionValueAgent/save.md) |
| `to(platform)` | `ActionValueAgent&` | [`to`](ActionValueAgent/to.md) |

## Changelog

- **0.1** -- `ActionValueAgent`: `learn` over `ActionValueBatch`,
  `predict`, `act`, `assess`, `save`/`load`, `ready`, `to`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

ActionValueAgent/Introduction
ActionValueAgent/act
ActionValueAgent/assess
ActionValueAgent/learn
ActionValueAgent/load
ActionValueAgent/predict
ActionValueAgent/ready
ActionValueAgent/save
ActionValueAgent/to
ActionValueAgentOptions
ActionValueTrainOptions
```
<!-- apiref:toctree:end -->
