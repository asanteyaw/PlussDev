# `pluss::ai::ice::models::TextJudgeReport`

The loss, accuracy, and example count
[`TextJudge::assess`](TextJudge/assess.md) returns.

## Signature

```cpp
pluss::ai::ice::models::TextJudgeReport::TextJudgeReport() = default;
```

A plain aggregate -- `loss`/`accuracy`/`count` are all `0` by
default.

## State

### `loss`

`double loss = 0.0`
: The mean training loss (cross-entropy) over the scored batch.

### `accuracy`

`double accuracy = 0.0`
: Fraction of examples whose predicted label matched the given
  label, in `[0, 1]`.

### `count`

`std::int64_t count = 0`
: Number of examples scored -- the length of the `texts`/`labels`
  (or `docs`/`labels`) arguments to [`assess`](TextJudge/assess.md).

## Methods

None -- `TextJudgeReport` is a plain read-only result value; see
`## State` above.

## Practical Examples

Three examples: `assess()`'s `count` matching the scored batch size
with a finite loss and an in-range accuracy, building one by hand,
and `accuracy` actually reflecting correctness -- higher when scored
against the real labels than against every label flipped.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <cmath>
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

  const auto report = model.assess(texts, labels);
  std::cout << "report.count = " << report.count << "\n";
  std::cout << "report.loss is finite = " << std::isfinite(report.loss) << "\n";
  std::cout << "report.accuracy in [0,1] = " << (report.accuracy >= 0.0 && report.accuracy <= 1.0) << "\n";

  return 0;
}
```

Output

```text
report.count = 96
report.loss is finite = 1
report.accuracy in [0,1] = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::TextJudgeReport report{};
  report.loss = 0.12;
  report.accuracy = 0.97;
  report.count = 96;

  std::cout << "loss = " << report.loss << "\n";
  std::cout << "accuracy = " << report.accuracy << "\n";
  std::cout << "count = " << report.count << "\n";

  return 0;
}
```

Output

```text
loss = 0.12
accuracy = 0.97
count = 96
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

  // Scoring against the fit corpus's own labels should give high accuracy;
  // scoring with every label flipped should give low accuracy -- the same
  // report type reflects both.
  const auto good_report = model.assess(texts, labels);

  std::vector<std::string> flipped_labels{};
  for (const auto& label : labels) {
    flipped_labels.push_back(label == "finance" ? "sport" : "finance");
  }
  const auto bad_report = model.assess(texts, flipped_labels);

  std::cout << "good_report.accuracy > bad_report.accuracy: "
            << (good_report.accuracy > bad_report.accuracy) << "\n";

  return 0;
}
```

Output

```text
good_report.accuracy > bad_report.accuracy: 1
```

## Changelog

- **0.1** -- `TextJudgeReport`: `loss`/`accuracy`/`count`.

## See Also

- [`TextJudge::assess`](TextJudge/assess.md) -- the only producer.
- [`TextJudgePrediction`](TextJudgePrediction.md) -- what
  [`TextJudge::predict`](TextJudge/predict.md) returns per input
  instead.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
