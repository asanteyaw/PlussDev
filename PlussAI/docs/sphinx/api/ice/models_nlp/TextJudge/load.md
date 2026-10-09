<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 385d62236ea9 -->
# `pluss::ai::ice::models::TextJudge::load`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
[[nodiscard]] static pluss::ai::ice::models::TextJudge pluss::ai::ice::models::TextJudge::load(
    std::string_view filepath);
```

## What It Does

The inverse of [`save`](save.md): reads `filepath`'s single text
artifact to reconstruct [`TextJudgeOptions`](../TextJudgeOptions.md),
the fitted label vocabulary (so [`labels()`](labels.md) round-trips
exactly -- see Example 3), and the trained classifier's own state,
with no caller-supplied options needed at all. The returned model is
already [`ready()`](ready.md) -- no call to [`learn`](learn.md) is
needed or expected.

## Parameters

`std::string_view filepath`
: The path originally passed to [`save`](save.md). Must be
  non-empty and refer to a file written by `save`.

## Returns

[`TextJudge`](../TextJudge.md), by value -- a fully independent
model, `ready()` and in evaluation mode.

## Errors

- Throws if `filepath` is empty, cannot be opened, or does not start
  with this model's own artifact header.

## Practical Examples

Three examples: an empty path being rejected immediately, a full
save/load round trip reproducing identical predictions, and the
label vocabulary round-tripping exactly even though the caller never
passes the original model to `load` at all.

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
    auto model = models::TextJudge::load("");
    static_cast<void>(model);
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
loaded.ready() = 1
save/load round-trip reproduces identical predictions: 1
```

### Example 3

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

  const auto path = std::filesystem::temp_directory_path() / "ice_textjudge_load_labels.artifact";
  model.save(path.string());

  // Loaded with no access to `model` at all -- its own label vocabulary is
  // reconstructed purely from the saved artifact.
  const auto loaded = models::TextJudge::load(path.string());
  std::filesystem::remove(path);

  std::cout << "loaded.labels() == model.labels(): " << (loaded.labels() == model.labels()) << "\n";

  return 0;
}
```

Output

```text
loaded.labels() == model.labels(): 1
```

## See Also

- [`save`](save.md) -- the inverse, and what artifact this reads.
- [`labels`](labels.md) -- round-trips exactly through this call.
- [`ready`](ready.md) -- always `true` on the returned model.

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
