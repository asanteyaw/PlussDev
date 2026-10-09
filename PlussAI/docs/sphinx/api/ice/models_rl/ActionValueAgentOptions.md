# `pluss::ai::ice::models::ActionValueAgentOptions`

Observation/action sizing, hidden width/depth, discount factor,
double-Q estimation, and target-network sync cadence for
[`ActionValueAgent`](ActionValueAgent.md)'s constructor.

## Fields

### `observation_size`

`std::int64_t observation_size = 0`
: Width of the observation vector fed to the Q-network.

### `action_count`

`std::int64_t action_count = 0`
: Number of discrete actions -- must be at least `2`.

### `hidden_size`

`std::int64_t hidden_size = 64`
: Hidden width of the Q-network.

### `hidden_layers`

`std::int64_t hidden_layers = 2`
: Number of hidden layers in the Q-network.

### `discount`

`double discount = 0.99`
: The reward discount factor (gamma), in `[0, 1]`, used when
  computing each transition's TD target.

### `double_estimate`

`bool double_estimate = true`
: Use double-DQN-style action selection (select the next action with
  the online network, evaluate it with the target network) rather
  than vanilla DQN's single-network max.

### `target_sync_every`

`std::int64_t target_sync_every = 16`
: Copy the online network's weights into the target network every
  this many optimizer steps (not epochs).

### `cfg`

`Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU)`
: Dtype and platform the model's own parameters are constructed on.

### `seed`

`std::uint64_t seed = 0`
: Seeds the model's own parameter initialization -- independent of
  [`ActionValueTrainOptions::seed`](ActionValueTrainOptions.md),
  which seeds shuffling/the train-validation split instead.

## Guidance

Leave every field at its default for a reasonable first run.
`target_sync_every` trades off stability (a larger value keeps the
TD target fixed longer, reducing a classic DQN instability source)
against how quickly the target network catches up to the online
network's own improvements -- lower it for faster-changing tasks,
raise it for noisier ones. There is no `from_data`/`from_dataset`
factory here -- `ActionValueAgent` has no `data::`-family
integration at all; build [`ActionValueBatch`](ActionValueBatch.md)
directly from transition tensors.

## See Also

- [`ActionValueAgent`](ActionValueAgent.md) -- the constructor this
  configures.
- [`ActionValueBatch`](ActionValueBatch.md) -- the transition batch
  shape `observation_size`/`action_count` describe.
