<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e036bca2da62 -->
# `pluss::ai::ice::models::ActionValueAgent::load`

[Back to `pluss::ai::ice::models::ActionValueAgent`](../ActionValueAgent.md)

## Signature

```cpp
[[nodiscard]] static pluss::ai::ice::models::ActionValueAgent pluss::ai::ice::models::ActionValueAgent::load(
    std::string_view filepath);
```

## What It Does

The inverse of [`save`](save.md): reads `filepath`'s header to
reconstruct an [`ActionValueAgentOptions`](../ActionValueAgentOptions.md)
with no caller-supplied options needed at all, constructs a fresh
agent from it, then loads `filepath + ".online.weights"` and
`filepath + ".target.weights"` into the online and target networks
respectively. The returned agent is already [`ready()`](ready.md) --
no call to [`learn`](learn.md) is needed or expected.

## Parameters

`std::string_view filepath`
: The header path originally passed to [`save`](save.md). Must be
  non-empty and refer to a file written by `save`; the two weights
  files are derived from it by appending `".online.weights"`/
  `".target.weights"`.

## Returns

[`ActionValueAgent`](../ActionValueAgent.md), by value -- a fully
independent agent, `ready()` and in evaluation mode.

## Errors

- Throws if `filepath` is empty, cannot be opened, or does not start
  with this model's own artifact header.
- Throws if either weights file is missing or does not match the
  parameter shapes the header describes.

## Practical Examples

Three examples: an empty path being rejected immediately, a full
save/load round trip reproducing identical predictions, and the
loaded agent enforcing its own reconstructed `observation_size` even
though the caller never passed any options to `load` at all.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  bool threw = false;
  try {
    auto agent = models::ActionValueAgent::load("");
    static_cast<void>(agent);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "load(\"\") throws = " << threw << "\n";

  return 0;
}
```

Output

```text
load("") throws = 1
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
loaded.ready() = 1
save/load round-trip reproduces identical predictions, diff = 0
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
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

  // observation_size = 2 -- not repeated anywhere near load().
  models::ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 8;

  models::ActionValueAgent agent(model_options);
  models::ActionValueTrainOptions train_options{};
  train_options.execution.max_epochs = 5;
  train_options.validation.split_ratio = 0.0;
  agent.learn(make_chain_batch(8), train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_ava_load_shapes.artifact";
  agent.save(path.string());

  // Loaded with no access to `model_options` at all -- its own
  // observation_size was reconstructed purely from the saved header.
  const auto loaded = models::ActionValueAgent::load(path.string());
  std::filesystem::remove(path);
  std::filesystem::remove(path.string() + ".online.weights");
  std::filesystem::remove(path.string() + ".target.weights");

  const auto f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto wrong_width = pluss::make_series(std::vector<float>{1.0f, 0.0f, 0.0f}, {1, 3}, f32);
  bool threw = false;
  try {
    const auto q = loaded.predict(wrong_width);
    static_cast<void>(q);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "loaded agent rejects the wrong observation width = " << threw << "\n";

  return 0;
}
```

Output

```text
loaded agent rejects the wrong observation width = 1
```

## See Also

- [`save`](save.md) -- the inverse, and what header/weights files
  this reads.
- [`ActionValueAgentOptions`](../ActionValueAgentOptions.md) -- every
  field reconstructed from the header.
- [`ready`](ready.md) -- always `true` on the returned agent.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="predict.html">
    <span class="label">Next</span>
    <span class="title">predict</span>
  </a>
</div>
<!-- apiref:nav:end -->
