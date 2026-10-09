<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6644c81a33db -->
# `pluss::ai::ice::models::TextJudge::assess`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::models::TextJudgeReport pluss::ai::ice::models::TextJudge::assess(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    pluss::RangeView<std::string_view> labels) const;
[[nodiscard]] pluss::ai::ice::models::TextJudgeReport pluss::ai::ice::models::TextJudge::assess(
    pluss::RangeView<std::string_view> texts,
    pluss::RangeView<std::string_view> labels) const;
[[nodiscard]] pluss::ai::ice::models::TextJudgeReport pluss::ai::ice::models::TextJudge::assess(
    const std::vector<std::string>& texts, const std::vector<std::string>& labels) const;
[[nodiscard]] pluss::ai::ice::models::TextJudgeReport pluss::ai::ice::models::TextJudge::assess(
    const pluss::Note& texts, const pluss::Note& labels) const;
```

## What It Does

Scores the model against `texts`/`labels`, returning a
[`TextJudgeReport`](../TextJudgeReport.md) -- mean loss, accuracy, and
example count. All four overloads mirror
[`learn`'s own four overloads](learn.md), and produce an identical
report for identical underlying text/label pairs (see Example 2).
`accuracy` genuinely reflects correctness -- scoring against the real
labels gives a higher accuracy than scoring the same texts against
every label flipped (see Example 3).

## Parameters

`const std::vector<pluss::ai::nlp::Doc>& docs` / `RangeView<std::string_view> texts` / `const std::vector<std::string>& texts` / `const pluss::Note& texts`
: The texts to score, in whichever of the four shapes is convenient.

`RangeView<std::string_view> labels` / `const std::vector<std::string>& labels` / `const pluss::Note& labels`
: The ground-truth label per text, same length as `texts`/`docs`.

## Returns

[`TextJudgeReport`](../TextJudgeReport.md).

## Errors

- Throws if `texts`/`docs` and `labels` have different lengths.

## Practical Examples

Three examples: a finite loss and in-range accuracy against the
training corpus, the `pluss::Note` overload matching the
`vector<std::string>` overload exactly, and `accuracy` dropping when
every label is deliberately flipped.

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

  const auto report_vec = model.assess(texts, labels);

  const pluss::Note note_texts = pluss::make_note(texts);
  const pluss::Note note_labels = pluss::make_note(labels);
  const auto report_note = model.assess(note_texts, note_labels);

  std::cout << "Note overload matches vector<string> overload exactly: "
            << (report_note.loss == report_vec.loss && report_note.accuracy == report_vec.accuracy &&
                report_note.count == report_vec.count)
            << "\n";

  return 0;
}
```

Output

```text
Note overload matches vector<string> overload exactly: 1
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

## See Also

- [`TextJudgeReport`](../TextJudgeReport.md) -- the returned type.
- [`learn`](learn.md) -- minimizes the same loss this reports.
- [`predict`](predict.md) -- per-input predictions instead of an
  aggregate report, for text without labels.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="labels.html">
    <span class="label">Next</span>
    <span class="title">labels</span>
  </a>
</div>
<!-- apiref:nav:end -->
