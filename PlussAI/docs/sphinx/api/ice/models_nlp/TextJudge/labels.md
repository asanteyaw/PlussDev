<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7224ad4ca9e6 -->
# `pluss::ai::ice::models::TextJudge::labels`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
[[nodiscard]] const std::vector<std::string>& pluss::ai::ice::models::TextJudge::labels() const noexcept;
```

## What It Does

Returns every distinct label value [`learn`](learn.md) saw, in
first-seen order (not alphabetical, and not insertion order of any
other kind -- see Example 2). Each
[`TextJudgePrediction::label`](../TextJudgePrediction.md) from
[`predict`](predict.md) is always one of these values. Empty before
the first `learn()`/[`load()`](load.md) call.

## Parameters

None.

## Returns

`const std::vector<std::string>&` -- a reference into this
`TextJudge`; valid as long as it is.

## Errors

None -- `noexcept`.

## Practical Examples

Three examples: `labels()` empty on a freshly constructed model,
`labels()` reporting values in first-seen order rather than
alphabetically, and `labels()` round-tripping exactly through
[`save`](save.md)/[`load`](load.md) with no access to the original
model.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::TextJudgeOptions model_options{};
  model_options.width = 16;
  models::TextJudge model(model_options);

  // labels() is empty before learn() -- nothing fitted yet.
  std::cout << "labels().empty() before learn() = " << model.labels().empty() << "\n";

  return 0;
}
```

Output

```text
labels().empty() before learn() = 1
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

  // "sport" appears first in the corpus, "finance" second -- labels()
  // should report them in that first-seen order, not alphabetically.
  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 12; ++i) {
    texts.push_back("the striker scored twice in the cup match");
    labels.push_back("sport");
    texts.push_back("stocks rallied after the earnings report");
    labels.push_back("finance");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 16;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.execution.max_epochs = 3;
  train_options.validation.split_ratio = 0.0;
  model.learn(texts, labels, train_options);

  std::cout << "labels().size() = " << model.labels().size() << "\n";
  std::cout << "labels()[0] = " << model.labels()[0] << "\n";
  std::cout << "labels()[1] = " << model.labels()[1] << "\n";

  return 0;
}
```

Output

```text
labels().size() = 2
labels()[0] = sport
labels()[1] = finance
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

- [`learn`](learn.md) -- the only way this gets populated directly.
- [`TextJudgePrediction::label`](../TextJudgePrediction.md) -- always
  one of these values.
- [`load`](load.md) -- the other way to populate this, from a saved
  artifact.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
