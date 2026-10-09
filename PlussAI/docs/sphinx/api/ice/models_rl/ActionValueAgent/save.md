<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a3fead928cb3 -->
# `pluss::ai::ice::models::ActionValueAgent::save`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
void pluss::ai::ice::models::ActionValueAgent::save(std::string_view filepath) const;
```

## What It Does

Writes **three** files: `filepath` itself (a small text header --
[`ActionValueAgentOptions`](../ActionValueAgentOptions.md)'s fields
plus the optimizer's own update-step count, enough to reconstruct
the architecture), `filepath + ".online.weights"`, and
`filepath + ".target.weights"` -- the online and target Q-networks'
own parameters, saved separately since double-DQN keeps two distinct
networks. All three are required by [`load`](load.md) to reconstruct
an equivalent, `ready()` agent.

## Parameters

`std::string_view filepath`
: Where to write the header; the two weights files are derived from
  it by appending `".online.weights"`/`".target.weights"`. Must be
  non-empty.

## Returns

`void`.

## Errors

- Throws if the agent has not been trained (`ready() == false`) --
  there is nothing meaningful to save yet.
- Throws if `filepath` is empty, or if any of the three files cannot
  be opened/written.

## Practical Examples

Three examples: `save` refusing to run before `learn`, a full save/
load round trip reproducing identical predictions across all three
files, and `filepath` still being validated even once the agent
itself is trained.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  models::ActionValueAgent agent(model_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_ava_save_untrained.artifact";
  bool threw = false;
  try {
    agent.save(path.string());
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "save() before learn() throws = " << threw << "\n";

  return 0;
}
```

Output

```text
save() before learn() throws = 1
```

### Example 2

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
  using pluss::ScalarType;

  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 16;
  model_options.seed = 21;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 20;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(32), train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_ava_save_roundtrip.artifact";
  agent.save(path.string());
  std::cout << "header file exists = " << std::filesystem::exists(path) << "\n";
  std::cout << "online weights file exists = " << std::filesystem::exists(path.string() + ".online.weights") << "\n";
  std::cout << "target weights file exists = " << std::filesystem::exists(path.string() + ".target.weights") << "\n";

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto stacked = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  const auto before = agent.predict(stacked);

  const auto loaded = models::ActionValueAgent::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".online.weights");
  std::filesystem::remove(path.string() + ".target.weights");

  std::cout << "loaded.ready() = " << loaded.ready() << "\n";
  const auto after = loaded.predict(stacked);
  const double diff = before.sub(after).abs().amax().item<double>();
  std::cout << "save/load round-trip reproduces identical predictions, diff = " << diff << "\n";

  return 0;
}
```

Output

```text
header file exists = 1
online weights file exists = 1
target weights file exists = 1
loaded.ready() = 1
save/load round-trip reproduces identical predictions, diff = 0
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

  models::ActionValueBatch batch{};
  batch.observation = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f, 1.0f}, {2, 2}, f32);
  batch.action = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2}, i64);
  batch.reward = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);
  batch.next_observation = pluss::make_series(std::vector<float>{0.0f, 1.0f, 0.0f, 0.0f}, {2, 2}, f32);
  batch.done = pluss::make_series(std::vector<float>{0.0f, 1.0f}, {2}, f32);

  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 2;
  train_options.validation.split_ratio = 0.0;
  agent.learn(batch, train_options);

  bool threw = false;
  try {
    agent.save("");
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "save(\"\") on a trained agent still throws = " << threw << "\n";

  return 0;
}
```

Output

```text
save("") on a trained agent still throws = 1
```

## See Also

- [`load`](load.md) -- the inverse.
- [`ready`](ready.md) -- what `save` requires to be `true` first.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ready.html">
    <span class="label">Previous</span>
    <span class="title">ready</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
<!-- apiref:nav:end -->
