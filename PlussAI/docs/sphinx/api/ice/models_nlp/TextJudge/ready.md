<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 97cff334f784 -->
# `pluss::ai::ice::models::TextJudge::ready`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
[[nodiscard]] bool pluss::ai::ice::models::TextJudge::ready() const noexcept;
```

## What It Does

Reports whether [`learn`](learn.md) has run at least once (directly,
or indirectly via [`load`](load.md)). `false` for a freshly
constructed model; `true` from then on, forever -- there is no way to
reset it short of constructing a new model. [`predict`](predict.md)/
[`assess`](assess.md)/[`save`](save.md) all require it to be `true`.

## Parameters

None.

## Returns

`bool`.

## Errors

None -- `noexcept`.

## Practical Examples

Three examples: the `false` -> `true` transition across `learn`,
`ready()` staying `true` across any number of subsequent `predict`/
`assess` calls, and a freshly `load`-ed model already reporting
`true` with no `learn` call at all.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

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
  std::cout << "ready() before learn() = " << model.ready() << "\n";

  models::TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 80;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 13;
  model.learn(texts, labels, train_options);
  std::cout << "ready() after learn() = " << model.ready() << "\n";

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

  // ready() stays true across repeated predict()/assess() calls.
  const auto preds = model.predict(texts);
  const auto report = model.assess(texts, labels);
  static_cast<void>(preds);
  static_cast<void>(report);
  std::cout << "ready() after predict()/assess() = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() after predict()/assess() = 1
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

  std::vector<std::string> texts{"alpha", "beta", "alpha", "beta"};
  std::vector<std::string> labels{"a", "b", "a", "b"};

  models::TextJudgeOptions model_options{};
  model_options.width = 8;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.execution.max_epochs = 3;
  train_options.validation.split_ratio = 0.0;
  model.learn(texts, labels, train_options);

  const auto path = std::filesystem::temp_directory_path() / "ice_textjudge_ready_load.artifact";
  model.save(path.string());

  const auto loaded = models::TextJudge::load(path.string());
  std::filesystem::remove(path);

  // ready() is true immediately -- load() never requires a follow-up
  // learn() call.
  std::cout << "loaded.ready() without calling learn() = " << loaded.ready() << "\n";

  return 0;
}
```

Output

```text
loaded.ready() without calling learn() = 1
```

## See Also

- [`learn`](learn.md) -- the only way to make a freshly constructed
  model `ready()`.
- [`load`](load.md) -- the other way, skipping `learn` entirely.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="predict.html">
    <span class="label">Previous</span>
    <span class="title">predict</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
