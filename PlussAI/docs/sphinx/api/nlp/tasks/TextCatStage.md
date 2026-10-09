# `pluss::ai::nlp::TextCatStage`

`TextCatStage` assigns a document-level label, and optionally several labels,
from labeled training documents. It writes its result to `Doc::doc_label` and
`Doc::doc_score`, or to `Doc::doc_labels` in multi-label mode. Unlike the
token-level stages, it describes the whole document: a topic, a category, a
priority, or any tag that applies to the text as a unit.

The stage has two backends that share one interface. The classic backend
represents each document as a weighted bag of words and fits a linear model
over that representation. The sequence backend learns a small embedding for
each word, averages the embeddings of a document's words, and fits a softmax
head over the average. The classic backend is the simpler one to start with,
and it has fewer settings to tune. The sequence backend can learn a richer
representation, and it is the only backend that supports multi-label output.

The stage reads tokens, so it runs after a tokenizer, and it reads lemmas when
they are present. It does not need a sentencizer or a tagger, although a
lemmatizer improves the classic features by merging inflected forms.

## Signature

```cpp
pluss::ai::nlp::TextCatStage::TextCatStage(
    pluss::ai::nlp::TextCatOptions options = {});
pluss::ai::nlp::TextCatStage::~TextCatStage();
void pluss::ai::nlp::TextCatStage::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    pluss::RangeView<std::string_view> labels,
    const pluss::ai::nlp::TrainOptions& options);
pluss::ai::nlp::Stage pluss::ai::nlp::stage::textcat(
    pluss::ai::nlp::TextCatOptions options = {});
```

## Construction

Use `stage::textcat` to place the stage in a workflow. Keep the concrete class
when you need to train it, and wrap it in a `Stage` for the workflow, or pass
the training labels through `Workflow::learn`, which forwards them to the stage.

The constructor stores the options and does no validation. Option errors
surface when training starts, because some combinations are only invalid for
one backend. A stage that has never been trained leaves `doc_label` empty and
`doc_score` as NaN.

## Options

The options struct is `pluss::ai::nlp::TextCatOptions`. The first group of
fields applies to the feature representation, and the second group applies to
training and to the backends.

### `lowercase`

Controls whether features are lowercased. When `true`, `Laurel` and `laurel`
count as one feature in both backends. When `false`, they are separate features.
Keep it on unless the case of a word is itself a signal, as with acronyms in a
corpus where the uppercase form always means something specific.

### `use_lemmas`

Controls whether a token's lemma is used as its feature when the lemma is
present. When `true`, `wrote`, `writes`, and `write` all contribute to the same
feature `write`. When `false`, each surface form is a separate feature. A
lemmatizer must run before this stage for the option to have any effect.

### `skip_punct`

Controls whether punctuation tokens are excluded from the features. When `true`,
commas and periods are never features, which keeps the vocabulary to words. When
`false`, punctuation can be a feature, which occasionally helps when the marks
carry style, such as exclamation points in promotional text.

### `max_features`

`std::int64_t`, default `4096`. The number of features the classic backend keeps.
The mapper ranks the candidate features by how many training documents contain
them, breaks ties alphabetically, and keeps the top `max_features`. The rest are
discarded before training, and they never contribute to a prediction. The sequence
backend's vocabulary is built from every term, so this option does not limit it.

### `min_document_frequency`

`std::int64_t`, default `1`. A feature is a candidate only if at least this many
training documents contain it. Raise it to drop words that appear in one or two
documents, which are usually names or typos that the model would overfit. If no
feature reaches the threshold, training fails. The Practical Examples show this
error.

### `max_iterations`

`std::int64_t`, default `400`. The most optimizer steps the classic backend runs
before it stops, even if the loss has not converged. It must be positive.

### `tolerance`

`double`, default `1e-6`. The classic backend stops early when the change in loss
between iterations falls below this value. It must be finite and nonnegative.
Lower it for a more exact fit, at the cost of more iterations.

### `multi_label`

`bool`, default `false`. When `true`, each training label is read as a `|`-joined set
of tags, so `tech|urgent` means the document has both tags. `apply` then writes every
tag whose probability reaches `multi_label_threshold` to `Doc::doc_labels`, sorted from
most to least probable, and leaves `Doc::doc_label` empty. Multi-label mode is valid
only with the Sequence backend, and training with the classic backend throws.

### `multi_label_threshold`

`double`, default `0.5`. The probability a tag must reach to be reported in multi-label
mode. Each tag is judged on its own, so a document can have several tags above the
threshold, or none. Raise it for fewer, more confident tags, and lower it when missing
a tag is worse than reporting a weak one.

### `backend`

`TaskBackend::Classic` uses the weighted bag of words and the linear model. It
supports single-label output only. `TaskBackend::Sequence` uses the learned
embedding and softmax head, and it supports both single-label and multi-label output.

### `sequence_width`

`std::int64_t`, default `64`. The width of the sequence backend's embedding, which is also
the width of the pooled document vector the head reads. It must be positive when the
Sequence backend trains. The classic backend ignores it.

### `sequence_learning_rate`

`double`, default `5e-2`. The step size of the sequence backend's optimizer. It must be
finite and positive. A rate that is too large makes training oscillate, and one that is
too small leaves the model near its initial, uninformed state, with a score close to
chance.

### `sequence_dropout`

`double`, default `0.0`. The fraction of activations randomly zeroed during training.
It must be in `[0, 1)`. Dropout helps when a few distinctive words dominate training and
the model memorizes them. It has no effect at prediction time.

### `sequence_seed`

`std::uint64_t`, default `0`. The seed for the sequence backend's initial weights and
for the randomness of its training loop. Two runs with the same seed and the same training documents
produce the same predictions.

## State

A trained stage keeps the feature mapper's vocabulary and document frequencies, the
classic model's weights, and the sequence backend's vocabulary, embedding, and head.
Its options are kept too.

`dump_state_lines()` writes the options and every trained component, and
`load_state_lines()` restores them. The mapper's options are written as their own
lines, so a restored stage builds the same features. A restored stage predicts the
same labels as the original.

## Methods

### `name()`

Returns `"textcat"`.

### `learn(docs, labels, options)`

Trains from the documents and one label per document. The label count must equal the
document count, the corpus must not be empty, and no label may be an empty string.
Training needs at least two distinct labels, in both backends. It throws otherwise.

With the classic backend, `learn` builds the feature mapper, fits a binary classifier
when there are two labels and a multi-class classifier when there are more, and stops
at `max_iterations` or `tolerance`. With the sequence backend, it builds the
vocabulary, initializes the embedding and head from `sequence_seed`, and runs a
seeded optimizer for the training loop, with dropout when `sequence_dropout` is set.

Calling `learn` replaces the previous training.

### `apply(Doc&)`

Clears `doc_label`, `doc_score`, and `doc_labels`, then predicts. An untrained stage
leaves all three cleared.

The classic backend always produces a label for a trained stage, even for a document
whose words are all unknown, because the model then reduces to its bias term. Its
`doc_score` is the probability of the chosen class: a sigmoid of the margin for two
labels, and a softmax over the class scores for more than two.

The sequence backend pools the embeddings of the document's known terms. When none of
the document's terms is in the vocabulary, it produces no label, leaves `doc_label`
empty, and sets `doc_score` to NaN. In single-label mode the score is the softmax
probability of the chosen label. In multi-label mode the stage fills `doc_labels`
instead, with one entry per tag at or above the threshold.

### `dump_state_lines()`

Returns the options and the trained components, in the form described under State.

### `load_state_lines(lines)`

Replaces the options and trained state from a saved set of lines. It validates the
restored components.

### `training_report()`

Returns the [`TrainingReport`](TrainingReport.md) of the most recent `learn` call: the
number of training documents, the number of features kept by the classic backend or
vocabulary terms of the Sequence backend, and the loss after each gradient step. The
classic backend's steps are its linear model's iterations, and for more than two labels
they cover each label's model in turn. An untrained stage returns an empty record.

## Practical Examples

These examples train the three modes on the same small corpus, show what each one
returns for known and unknown text, and show the two training errors callers meet
first.

### Example 1

The classic backend on two topics. The model labels a document with words from
its training topics, and it labels the document with unknown words too, because it
always returns its best class. The score shows how confident the model is.

```cpp
#include <pluss/nlp.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  auto textcat = std::make_shared<nlp::TextCatStage>(nlp::TextCatOptions{});
  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), textcat}};

  const std::vector<std::string> texts = {
      "The API build failed in CI.",
      "Write the docs for the API.",
      "Bake bread with flour and water.",
      "Cook pasta and bake a cake.",
  };
  const std::vector<std::string> labels = {"tech", "tech", "food", "food"};
  wf.learn(texts, labels, nlp::TrainOptions{});
  const nlp::TrainingReport report = textcat->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";


  for (const std::string text : {"The CI build broke.", "Flour and water make bread.", "zebra quartz"}) {
    const nlp::Doc doc = wf.infer(text);
    std::cout << "\"" << text << "\" -> label = " << doc.doc_label << ", score = " << doc.doc_score << "\n";
  }
  return 0;
}
```

Output

```text
docs = 4, entries = 19, steps = 400
final loss = 0.162366
"The CI build broke." -> label = tech, score = 0.790782
"Flour and water make bread." -> label = food, score = 0.797037
"zebra quartz" -> label = food, score = 0.509796
```

### Example 2

The sequence backend on the same corpus, with a seed so the result is repeatable.
It labels known text with a softmax score. Text whose words are all unknown gets no
label and a NaN score, which is the backend's way of saying it has nothing to base
a decision on.

```cpp
#include <pluss/nlp.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextCatOptions options{.backend = nlp::TaskBackend::Sequence, .sequence_width = 16, .sequence_seed = 7};
  auto textcat = std::make_shared<nlp::TextCatStage>(options);
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), textcat}};

  const std::vector<std::string> texts = {
      "The API build failed in CI.",
      "Write the docs for the API.",
      "Bake bread with flour and water.",
      "Cook pasta and bake a cake.",
  };
  const std::vector<std::string> labels = {"tech", "tech", "food", "food"};
  wf.learn(texts, labels, nlp::TrainOptions{.random = {.seed = 3}});
  const nlp::TrainingReport report = textcat->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";


  const nlp::Doc known = wf.infer("Bake bread and cake.");
  std::cout << "known: label = " << known.doc_label << ", score = " << known.doc_score << "\n";

  const nlp::Doc unknown = wf.infer("zebra quartz plasma");
  std::cout << "unknown: label = \"" << unknown.doc_label << "\", score_is_nan = " << std::isnan(unknown.doc_score) << "\n";
  return 0;
}
```

Output

```text
docs = 4, entries = 19, steps = 25
final loss = 1.43051e-06
known: label = food, score = 0.999999
unknown: label = "", score_is_nan = 1
```

### Example 3

Multi-label output. Each training label lists its tags separated by `|`, and the
prediction reports every tag above the threshold, from most to least probable. The
single-label field stays empty, because the document has no single label.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextCatOptions options{.backend = nlp::TaskBackend::Sequence,
                              .multi_label = true,
                              .multi_label_threshold = 0.5,
                              .sequence_width = 16,
                              .sequence_seed = 7};
  auto textcat = std::make_shared<nlp::TextCatStage>(options);
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), textcat}};

  const std::vector<std::string> texts = {
      "Urgent API build failed.",
      "Write API docs.",
      "Urgent bread recipe.",
      "Bread and cake recipe.",
  };
  const std::vector<std::string> labels = {"tech|urgent", "tech", "food|urgent", "food"};
  wf.learn(texts, labels, nlp::TrainOptions{.random = {.seed = 3}});
  const nlp::TrainingReport report = textcat->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";


  const nlp::Doc doc = wf.infer("Urgent API docs.");
  std::cout << "single label = \"" << doc.doc_label << "\"\n";
  std::cout << "tags:\n";
  for (const auto& [tag, probability] : doc.doc_labels) {
    std::cout << "  " << tag << " = " << probability << "\n";
  }
  return 0;
}
```

Output

```text
docs = 4, entries = 10, steps = 53
final loss = 3.02557e-05
single label = ""
tags:
  tech = 0.999789
  urgent = 0.91264
```

### Example 4

Two errors that callers meet early. Multi-label output with the classic backend
is rejected when training starts, because that combination has no meaning in this
stage. A minimum document frequency that no feature can meet also fails, because
the mapper has nothing to keep. The program prints the first line of each message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  const std::vector<std::string> texts = {"Bake bread.", "Cook pasta."};
  const std::vector<std::string> labels = {"food", "food|tech"};
  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};

  nlp::Workflow classic_multi{.lang = nlp::Language::English,
                              .stages = {nlp::stage::tokenize(),
                                         nlp::stage::textcat(nlp::TextCatOptions{.multi_label = true})}};
  try {
    classic_multi.learn(texts, labels, nlp::TrainOptions{});
    std::cout << "trained\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }

  nlp::Workflow strict{.lang = nlp::Language::English,
                       .stages = {nlp::stage::tokenize(),
                                  nlp::stage::textcat(nlp::TextCatOptions{.min_document_frequency = 5})}};
  try {
    strict.learn(std::vector<std::string>{"Bake bread.", "Cook pasta."},
                 std::vector<std::string>{"food", "tech"}, nlp::TrainOptions{});
    std::cout << "trained\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
nlp::TextCatStage::learn: multi_label is only supported with TaskBackend::Sequence.
nlp::TextCatStage::learn: LexiconMapper::learn found no tokens passing min_document_frequency.
```

## See Also

- `stage::textcat` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`RuleTokenizerStage`](RuleTokenizerStage.md) -- supplies the tokens the features are built from.
- [`LemmatizerStage`](LemmatizerStage.md) -- supplies the lemmas that `use_lemmas` reads.
- [`Doc`](../core/Doc.md) -- the `doc_label`, `doc_score`, and `doc_labels` fields this stage writes.
- [`Workflow::learn`](../workflow/Workflow/learn.md) -- trains every trainable stage in a pipeline.

## Changelog

- **0.1** -- initial documented surface.
- **0.1 (fix)** -- `sequence_learning_rate` now controls the single-label Sequence trainer; the rate was previously fixed at `0.05`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
