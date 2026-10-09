<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5142d93383da -->

# `pluss::ai::nlp::Workflow::learn`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const std::vector<std::string>& texts, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const std::vector<std::string>& texts, const std::vector<std::string>& labels, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const pluss::Note& texts, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const pluss::Note& texts, const pluss::Note& labels, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const std::vector<pluss::ai::nlp::Doc>& docs, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(const std::vector<pluss::ai::nlp::Doc>& docs, pluss::RangeView<std::string_view> labels, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(pluss::RangeView<std::string_view> texts, const pluss::ai::nlp::TrainOptions& options = {});
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::learn(pluss::RangeView<std::string_view> texts, pluss::RangeView<std::string_view> labels, const pluss::ai::nlp::TrainOptions& options = {});
```

## What It Does

Trains the workflow's trainable stages. The workflow first runs the texts (or documents) through every stage that comes before each trainable one, then hands the prepared documents to each stage's `learn`. Stages that do not learn ignore the call, so `learn` is safe on any pipeline. Overloads come in two shapes. Without labels the stages learn from the text itself. With labels, one label per document is supplied, as a text classifier needs. Each shape accepts raw strings, a `pluss::Note`, or documents already produced by `infer`, and the label container can match the text container (vector, `Note`, or a braced list). Learning changes the workflow in place and returns it for chaining.

## Parameters

`texts` / `docs`
: The training input. Strings, a `pluss::Note` of text, or documents already run through the pipeline's earlier stages.

`labels`
: One label per document, in the same order. A label container must have exactly as many entries as there are documents.

`options`
: A `TrainOptions`. Its `random` field holds the seed and determinism flag, and `allow_empty_docs` controls whether an empty input text is accepted (default `true`).

## Returns

A reference to this workflow, now trained.

## Errors

Throws when an empty document is supplied and `allow_empty_docs` is `false`. Throws when labels and documents differ in count.

## Practical Examples

### Example 1

Training a text classifier from labeled strings, then classifying new text.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::textcat()}};
  wf.learn(std::vector<std::string>{"Compiler errors break builds", "C++ headers need cleanup", "Fresh bread and soup", "Bake a sweet cake"}, std::vector<std::string>{"tech", "tech", "food", "food"});
  const auto predicted = wf.infer(std::vector<std::string>{"Headers and compiler warnings", "Bread soup and dinner"});
  std::cout << predicted[0].doc_label << " " << predicted[1].doc_label << "\n";
  return 0;
}
```

Output

```text
tech food
```

### Example 2

The same classifier trained from a `pluss::Note` of texts and a `Note` of labels.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::textcat()}};
  wf.learn(pluss::make_note({"Compiler errors break builds", "C++ headers need cleanup", "Fresh bread and soup", "Bake a sweet cake"}),
           pluss::make_note({"tech", "tech", "food", "food"}));
  const nlp::Doc doc = wf.infer(std::string_view("Bread soup and dinner"));
  std::cout << doc.doc_label << "\n";
  return 0;
}
```

Output

```text
food
```

### Example 3

Training from documents that were already tokenized, with labels given as a braced list.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::textcat()}};
  const std::vector<nlp::Doc> prepared = wf.infer(std::vector<std::string>{"Compiler errors break builds", "C++ headers need cleanup", "Fresh bread and soup", "Bake a sweet cake"});
  wf.learn(prepared, {"tech", "tech", "food", "food"});
  const nlp::Doc doc = wf.infer(std::string_view("Compiler headers"));
  std::cout << doc.doc_label << "\n";
  return 0;
}
```

Output

```text
tech
```

### Example 4

An empty training text is rejected when `allow_empty_docs` is off.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  bool threw = false;
  try {
    wf.learn(std::vector<std::string>{"hello", ""}, nlp::TrainOptions{.allow_empty_docs = false});
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

### Example 5

With the default options an empty training text is accepted.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  wf.learn(std::vector<std::string>{"hello", ""});
  std::cout << "learned=1\n";
  return 0;
}
```

Output

```text
learned=1
```

## See Also

- [`infer`](infer.md) -- running a trained pipeline.
- [`IStage::learn`](../../core/IStage/learn.md) -- what each trainable stage receives.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
