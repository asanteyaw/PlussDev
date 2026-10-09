# `pluss::ai::nlp::TrainingReport`

The record of the most recent `learn()` call on a trainable stage: how many
documents it saw, how many entries it learned, and, for the Sequence backend, the
loss after every optimizer step. The three trainable stages return it from
`training_report()`: [`TaggerStage`](TaggerStage.md), [`NerStage`](NerStage.md), and
[`TextCatStage`](TextCatStage.md). It is plain data, and calling code reads its fields.

A stage that has not been trained returns an empty record, and each `learn()` call
replaces the previous one. The record is not saved with the stage, so a loaded stage
reports empty until it is trained again.

## Signature

```cpp
pluss::ai::nlp::TrainingReport::TrainingReport() = default;
```

## State

### `doc_count`

`std::int64_t doc_count = 0`
: The number of documents the training call received.

### `entry_count`

`std::int64_t entry_count = 0`
: What the training produced, in units that depend on the stage. For the tagger it is
  the number of learned form, suffix, and word-shape entries. For NER it is the number
  of learned phrases across all widths. For text classification it is the number of
  features kept by the classic backend, or the number of vocabulary terms of the
  Sequence backend.

### `loss_history`

`std::vector<double> loss_history`
: The loss after each gradient step, in order. For the text classifier this covers both
  backends: the Sequence backend's optimizer steps, or the classic linear model's
  iterations. For more than two labels the classic classifier trains one model per label,
  so its history is each label's losses in turn. The tagger and NER classic backends
  learn count tables with no iterations, so their history is empty.

## Methods

### `steps()`

`std::int64_t steps() const noexcept`
: The number of gradient steps, which is the length of `loss_history`. Zero for the
  tagger and NER classic backends. For a text classifier the count can be lower than
  the configured `max_iterations`, because training stops early once the loss change
  falls below `tolerance`.

## Guidance

Read `loss_history` to see whether the Sequence backend converged. A curve that falls
and levels off is a sign of convergence. A curve that stops early, or one that is still
falling at the last step, suggests raising the iteration limit or the learning rate.

Read `entry_count` against `doc_count` to judge how much the stage learned from the data.
A tagger with a handful of entries learned from a handful of documents has little to
generalize from, whatever its loss says.

Copy the record before training again if you want to compare runs, because the next
`learn()` call replaces it.

## See Also

- [`TaggerStage`](TaggerStage.md), [`NerStage`](NerStage.md), and [`TextCatStage`](TextCatStage.md) -- each returns this record from `training_report()`.
- [`ice::TrainingReport`](../../ice/options/TrainingReport.md) -- the equivalent record for the generic `ice` training loop.

## Practical Examples

Three examples: the report of a classic tagger, the loss curve of a Sequence tagger,
and the step count of a text classifier that stopped early.

### Example 1

A classic tagger has entries and documents but no optimizer steps.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::vector<nlp::Doc> train;
  for (const char* text : {"Ada writes books.", "Kofi reads notes."}) {
    nlp::Doc doc = base.infer(text);
    const char* tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
    for (std::size_t i = 0; i < doc.tokens.size(); ++i) doc.tokens[i].pos = tags[i];
    train.push_back(doc);
  }

  nlp::TaggerStage tagger{};
  tagger.learn(train, {});
  const nlp::TrainingReport report = tagger.training_report();
  std::cout << "doc_count = " << report.doc_count << "\n";
  std::cout << "entry_count = " << report.entry_count << "\n";
  std::cout << "steps = " << report.steps() << "\n";
  return 0;
}
```

Output

```text
doc_count = 2
entry_count = 16
steps = 0
```

### Example 2

The Sequence tagger's loss curve, printed at a few points so the decline is visible.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  std::vector<nlp::Doc> train;
  for (const char* text : {"Ada writes books.", "Kofi reads notes."}) {
    nlp::Doc doc = base.infer(text);
    const char* tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
    for (std::size_t i = 0; i < doc.tokens.size(); ++i) doc.tokens[i].pos = tags[i];
    train.push_back(doc);
  }

  nlp::TaggerStage tagger{nlp::TaggerOptions{.backend = nlp::TaskBackend::Sequence, .sequence_width = 16, .sequence_seed = 5}};
  tagger.learn(train, {});
  const nlp::TrainingReport report = tagger.training_report();
  std::cout << "steps = " << report.steps() << "\n";
  for (const std::size_t index : {std::size_t{0}, report.loss_history.size() / 2, report.loss_history.size() - 1}) {
    std::cout << "step " << index + 1 << ": loss = " << report.loss_history[index] << "\n";
  }
  return 0;
}
```

Output

```text
steps = 60
step 1: loss = 1.3968
step 31: loss = 0.125624
step 60: loss = 0.0201613
```

### Example 3

A text classifier on the Sequence backend, with a tolerance loose enough that the loop
stops before its iteration limit. The step count is below the limit, which shows
the early stop.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::TextCatOptions options{.backend = nlp::TaskBackend::Sequence,
                              .max_iterations = 400,
                              .tolerance = 1e-3,
                              .sequence_width = 16,
                              .sequence_seed = 7};
  nlp::TextCatStage classifier{options};
  classifier.learn({wf.infer("Bake bread."), wf.infer("Cook pasta."), wf.infer("Write docs."), wf.infer("Fix build.")},
                   {"food", "food", "tech", "tech"}, nlp::TrainOptions{});
  const nlp::TrainingReport report = classifier.training_report();
  std::cout << "max_iterations = " << options.max_iterations << ", steps = " << report.steps() << "\n";
  std::cout << "entry_count = " << report.entry_count << ", doc_count = " << report.doc_count << "\n";
  return 0;
}
```

Output

```text
max_iterations = 400, steps = 16
entry_count = 8, doc_count = 4
```

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
