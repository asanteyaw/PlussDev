<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 12c6a42ad8b2 -->
# `pluss::ai::ice::models::TextJudge::predict`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
[[nodiscard]] std::vector<pluss::ai::ice::models::TextJudgePrediction> pluss::ai::ice::models::TextJudge::predict(
    const std::vector<pluss::ai::nlp::Doc>& docs) const;
[[nodiscard]] std::vector<pluss::ai::ice::models::TextJudgePrediction> pluss::ai::ice::models::TextJudge::predict(
    pluss::RangeView<std::string_view> texts) const;
[[nodiscard]] std::vector<pluss::ai::ice::models::TextJudgePrediction> pluss::ai::ice::models::TextJudge::predict(
    const std::vector<std::string>& texts) const;
[[nodiscard]] std::vector<pluss::ai::ice::models::TextJudgePrediction> pluss::ai::ice::models::TextJudge::predict(
    const pluss::Note& texts) const;
```

## What It Does

Runs the model in evaluation mode and returns one
[`TextJudgePrediction`](../TextJudgePrediction.md) per input, in
order. All four overloads accept the input corpus in a different
shape, exactly matching [`learn`'s own four overloads](learn.md) --
and produce identical predictions for the identical underlying texts,
regardless of which shape is used (see Examples 2 and 3).

## Parameters

`const std::vector<pluss::ai::nlp::Doc>& docs` / `RangeView<std::string_view> texts` / `const std::vector<std::string>& texts` / `const pluss::Note& texts`
: The texts to classify, in whichever of the four shapes is
  convenient.

## Returns

`std::vector<TextJudgePrediction>`, same length and order as the
input.

## Errors

None directly, beyond what a malformed input range would throw while
being read.

## Practical Examples

Three examples: the `vector<std::string>` overload returning one
prediction per input, the `pluss::Note` overload matching it exactly
on an identical probe set, and the `RangeView<std::string_view>`
overload matching it too.

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

  const std::vector<std::string> probes{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto preds_vec = model.predict(probes);

  const pluss::Note note_probes = pluss::make_note(probes);
  const auto preds_note = model.predict(note_probes);

  std::cout << "Note overload matches vector<string> overload: "
            << (preds_note[0].label == preds_vec[0].label && preds_note[1].label == preds_vec[1].label) << "\n";

  return 0;
}
```

Output

```text
Note overload matches vector<string> overload: 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <string_view>
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

  const std::vector<std::string> probes{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto preds_vec = model.predict(probes);

  const std::vector<std::string_view> view_probes(probes.begin(), probes.end());
  const auto preds_view = model.predict(view_probes);

  std::cout << "RangeView overload matches vector<string> overload: "
            << (preds_view[0].label == preds_vec[0].label && preds_view[1].label == preds_vec[1].label) << "\n";

  return 0;
}
```

Output

```text
RangeView overload matches vector<string> overload: 1
```

## See Also

- [`TextJudgePrediction`](../TextJudgePrediction.md) -- the returned
  element type.
- [`learn`](learn.md) -- trains the model this call reads.
- [`assess`](assess.md) -- a loss/accuracy summary instead of
  per-input predictions, for text that does have real labels.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
