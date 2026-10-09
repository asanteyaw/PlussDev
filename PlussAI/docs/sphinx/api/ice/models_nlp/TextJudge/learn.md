<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 28b1f6053313 -->
# `pluss::ai::ice::models::TextJudge::learn`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
void pluss::ai::ice::models::TextJudge::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    pluss::RangeView<std::string_view> labels,
    const TextJudgeTrainOptions& options = {});
void pluss::ai::ice::models::TextJudge::learn(
    pluss::RangeView<std::string_view> texts,
    pluss::RangeView<std::string_view> labels,
    const TextJudgeTrainOptions& options = {});
void pluss::ai::ice::models::TextJudge::learn(
    const std::vector<std::string>& texts, const std::vector<std::string>& labels,
    const TextJudgeTrainOptions& options = {});
void pluss::ai::ice::models::TextJudge::learn(
    const pluss::Note& texts, const pluss::Note& labels,
    const TextJudgeTrainOptions& options = {});
```

## What It Does

Trains the model in place on `texts`/`labels`, leaving it
[`ready()`](ready.md) and [`labels()`](labels.md) populated with every
distinct label value seen. All four overloads accept the corpus in a
different shape -- a pre-tokenized `Doc` list, an `nlp`/`msharp`
range view, a plain `std::vector<std::string>`, or a `pluss::Note` --
but produce the same trained model for the same corpus (see Example
2). [`TextJudgeOptions::use_lemmas`/`skip_punct`/`lowercase`](../TextJudgeOptions.md)
each change what the tokenizer actually hands the model; a label pair
distinguishable only by punctuation needs `skip_punct = false` to
learn at all (see Example 3).

## Parameters

`const std::vector<pluss::ai::nlp::Doc>& docs` / `RangeView<std::string_view> texts` / `const std::vector<std::string>& texts` / `const pluss::Note& texts`
: The training corpus, in whichever of the four shapes is convenient.

`RangeView<std::string_view> labels` / `const std::vector<std::string>& labels` / `const pluss::Note& labels`
: One label per text, same length as `texts`/`docs`.

`const TextJudgeTrainOptions& options`
: Batch size, learning rate, optimizer, and the shared validation/
  regularization/execution policies -- see
  [`TextJudgeTrainOptions`](../TextJudgeTrainOptions.md) field by
  field.

## Returns

`void` -- mutates the model in place; [`ready()`](ready.md) becomes
`true` afterward.

## Errors

- Throws if `texts`/`docs` and `labels` have different lengths.
- Throws if `labels` contains fewer than two distinct values --
  `TextJudge` is a classifier and needs at least two classes to
  learn.
- Throws per [`ExecutionPolicy::fail_on_non_finite`](../../options/ExecutionPolicy.md)
  if a non-finite loss/gradient appears during training.

## Practical Examples

Three examples: the `vector<std::string>` overload taking a model
from un-trained to `ready()`, the `pluss::Note` overload producing
the same trained predictions as the `vector<std::string>` overload on
an identical corpus, and `skip_punct = false` recovering a label pair
distinguishable only by punctuation.

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
  std::cout << "labels().size() = " << model.labels().size() << "\n";

  return 0;
}
```

Output

```text
ready() before learn() = 0
ready() after learn() = 1
labels().size() = 2
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <vector>

namespace {
pluss::ai::ice::models::TextJudgeOptions make_options() {
  pluss::ai::ice::models::TextJudgeOptions options{};
  options.width = 24;
  options.dropout = 0.0;
  options.seed = 9;
  return options;
}
pluss::ai::ice::models::TextJudgeTrainOptions make_train_options() {
  pluss::ai::ice::models::TextJudgeTrainOptions options{};
  options.batch_size = 12;
  options.learning_rate = 5e-2;
  options.execution.max_epochs = 80;
  options.validation.split_ratio = 0.0;
  options.seed = 13;
  return options;
}
} // namespace

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

  // Route A: the vector<std::string> overload.
  models::TextJudge model_a(make_options());
  model_a.learn(texts, labels, make_train_options());

  // Route B: the pluss::Note overload -- the same corpus, as a Note instead.
  const pluss::Note note_texts = pluss::make_note(texts);
  const pluss::Note note_labels = pluss::make_note(labels);
  models::TextJudge model_b(make_options());
  model_b.learn(note_texts, note_labels, make_train_options());

  const std::vector<std::string> probes{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto preds_a = model_a.predict(probes);
  const auto preds_b = model_b.predict(probes);
  std::cout << "Note overload matches vector<string> overload: "
            << (preds_a[0].label == preds_b[0].label && preds_a[1].label == preds_b[1].label) << "\n";

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
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  // "alpha !" vs "alpha ?" differ only by punctuation -- distinguishable
  // only when skip_punct is turned off.
  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("alpha !");
    labels.push_back("excited");
    texts.push_back("alpha ?");
    labels.push_back("curious");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 16;
  model_options.use_lemmas = false;
  model_options.skip_punct = false;
  model_options.lowercase = true;
  model_options.dropout = 0.0;
  model_options.seed = 21;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 40;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 33;
  model.learn(texts, labels, train_options);

  const auto preds = model.predict(std::vector<std::string>{"alpha !", "alpha ?"});
  std::cout << "preds[0].label = " << preds[0].label << "\n";
  std::cout << "preds[1].label = " << preds[1].label << "\n";

  return 0;
}
```

Output

```text
preds[0].label = excited
preds[1].label = curious
```

## See Also

- [`TextJudgeOptions`](../TextJudgeOptions.md) -- the tokenization
  toggles, including `skip_punct`.
- [`TextJudgeTrainOptions`](../TextJudgeTrainOptions.md) -- every
  training knob.
- [`predict`](predict.md) / [`assess`](assess.md) -- the usual next
  calls on a trained model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="labels.html">
    <span class="label">Previous</span>
    <span class="title">labels</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
