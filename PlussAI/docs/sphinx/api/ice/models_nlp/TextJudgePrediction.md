# `pluss::ai::ice::models::TextJudgePrediction`

One predicted label plus its confidence, returned per input by
[`TextJudge::predict`](TextJudge/predict.md).

## Signature

```cpp
pluss::ai::ice::models::TextJudgePrediction::TextJudgePrediction() = default;
```

A plain aggregate -- `label` is empty and `confidence` is `0.0` by
default.

## State

### `label`

`std::string label = {}`
: The predicted label -- one of the strings seen in
  [`TextJudge::learn`](TextJudge/learn.md)'s own `labels` argument,
  also listed by [`TextJudge::labels()`](TextJudge/labels.md).

### `confidence`

`double confidence = 0.0`
: The predicted label's own probability -- for a binary task, the
  winning label's confidence is always `> 0.5` (see Example 3).

## Methods

None -- `TextJudgePrediction` is a plain read-only result value; see
`## State` above.

## Practical Examples

Three examples: `predict()` returning one prediction per input text,
building one by hand, and confirming the winning label's confidence
is always above `0.5` for a binary task.

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

  models::TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 80;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 13;
  model.learn(texts, labels, train_options);

  const auto preds = model.predict(std::vector<std::string>{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  });
  std::cout << "preds.size() = " << preds.size() << "\n";
  std::cout << "preds[0].label = " << preds[0].label << "\n";
  std::cout << "preds[1].label = " << preds[1].label << "\n";

  return 0;
}
```

Output

```text
preds.size() = 2
preds[0].label = finance
preds[1].label = sport
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::TextJudgePrediction prediction{};
  prediction.label = "finance";
  prediction.confidence = 0.87;

  std::cout << "label = " << prediction.label << "\n";
  std::cout << "confidence = " << prediction.confidence << "\n";

  return 0;
}
```

Output

```text
label = finance
confidence = 0.87
```

### Example 3

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

  // confidence is the predicted label's own probability -- for a binary
  // task, the winning label's confidence is always > 0.5.
  const auto preds = model.predict(std::vector<std::string>{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  });
  std::cout << "preds[0].confidence > 0.5 = " << (preds[0].confidence > 0.5) << "\n";
  std::cout << "preds[1].confidence > 0.5 = " << (preds[1].confidence > 0.5) << "\n";

  return 0;
}
```

Output

```text
preds[0].confidence > 0.5 = 1
preds[1].confidence > 0.5 = 1
```

## Changelog

- **0.1** -- `TextJudgePrediction`: `label`/`confidence`.

## See Also

- [`TextJudge::predict`](TextJudge/predict.md) -- the only producer.
- [`TextJudge::labels`](TextJudge/labels.md) -- every value `label`
  can take.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
