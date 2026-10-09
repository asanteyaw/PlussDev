<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 948b982fe982 -->
# `pluss::ai::ice::models::TextJudge::save`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
void pluss::ai::ice::models::TextJudge::save(std::string_view filepath) const;
```

## What It Does

Writes `filepath` as a single text artifact: every
[`TextJudgeOptions`](../TextJudgeOptions.md) field, the fitted label
vocabulary, and the trained classifier's own state, all serialized as
text lines. Unlike
[`AutoregressiveForecaster::save`](../../models_ts/AutoregressiveForecaster/save.md)/[`TemporalFusionTransformer::save`](../../models_ts/TemporalFusionTransformer/save.md),
there is no separate `".weights"` sidecar file (see Example 2).

## Parameters

`std::string_view filepath`
: Where to write the artifact. Must be non-empty.

## Returns

`void`.

## Errors

- Throws if the model has not been trained (`ready() == false`) --
  there is nothing meaningful to save yet.
- Throws if `filepath` is empty, or if the file cannot be opened/
  written.

## Practical Examples

Three examples: `save` refusing to run before `learn`, a full save/
load round trip into a single file with no `.weights` sidecar, and
`filepath` still being validated even once the model itself is
trained.

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

  models::TextJudgeOptions model_options{};
  model_options.width = 16;
  models::TextJudge model(model_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_textjudge_save_untrained.artifact";
  bool threw = false;
  try {
    model.save(path.string());
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
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("stocks rallied after the earnings report");
    labels.push_back("finance");
    texts.push_back("the market closed higher on strong profits");
    labels.push_back("finance");
    texts.push_back("the striker scored twice in the cup match");
    labels.push_back("sport");
    texts.push_back("fans celebrated the league victory downtown");
    labels.push_back("sport");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 24;
  model_options.dropout = 0.0;
  model_options.seed = 9;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 80;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 13;
  model.learn(texts, labels, train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_textjudge_save_roundtrip.artifact";
  model.save(path.string());
  // A single file -- unlike AutoregressiveForecaster/TemporalFusionTransformer,
  // there is no separate ".weights" sidecar.
  std::cout << "artifact file exists = " << std::filesystem::exists(path) << "\n";
  std::cout << "no .weights sidecar exists = " << !std::filesystem::exists(path.string() + ".weights") << "\n";

  const std::vector<std::string> probes{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto before = model.predict(probes);
  const auto loaded = models::TextJudge::load(path.string());
  std::filesystem::remove(path);

  std::cout << "loaded.ready() = " << loaded.ready() << "\n";
  const auto after = loaded.predict(probes);
  std::cout << "save/load round-trip reproduces identical predictions: "
            << (before[0].label == after[0].label && before[0].confidence == after[0].confidence &&
                before[1].label == after[1].label && before[1].confidence == after[1].confidence)
            << "\n";

  return 0;
}
```

Output

```text
artifact file exists = 1
no .weights sidecar exists = 1
loaded.ready() = 1
save/load round-trip reproduces identical predictions: 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("alpha");
    labels.push_back("a");
    texts.push_back("beta");
    labels.push_back("b");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 8;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.execution.max_epochs = 3;
  train_options.validation.split_ratio = 0.0;
  model.learn(texts, labels, train_options);

  bool threw = false;
  try {
    model.save("");
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "save(\"\") on a trained model still throws = " << threw << "\n";

  return 0;
}
```

Output

```text
save("") on a trained model still throws = 1
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
