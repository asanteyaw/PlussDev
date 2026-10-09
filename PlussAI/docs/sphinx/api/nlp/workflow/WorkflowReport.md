# `pluss::ai::nlp::WorkflowReport`

What [`Workflow::assess`](Workflow/assess.md) returns: corpus-level counts
(documents, tokens, sentences) and their per-document means. It is a
pipeline-health summary: how much text went in and how it was split. It is
not a task-specific accuracy score.

## Fields

### `doc_count`

`std::int64_t`, default `0`. The number of documents the pipeline processed.

### `token_count`

`std::int64_t`, default `0`. The total number of tokens across all documents.

### `sentence_count`

`std::int64_t`, default `0`. The total number of sentences across all
documents. Zero when no sentencizer ran.

### `mean_tokens_per_doc`

`double`, default `0.0`. `token_count / doc_count`. Use it to spot a corpus
whose documents are far longer or shorter than expected.

### `mean_sentences_per_doc`

`double`, default `0.0`. `sentence_count / doc_count`. Like the token mean, it
reflects how the stages split the text, not how a model performs.

## Guidance

Read the report after changing stages or options to confirm the splitting
behaves as expected: a sudden drop in `mean_sentences_per_doc` often means a
sentencizer option changed. The report summarizes the pipeline, so compare it
across runs of the same corpus, not against a different corpus.

## See Also

- [`Workflow::assess`](Workflow/assess.md) -- produces the report.

## Practical Examples

### Example 1

Assessing two documents reports their counts and means.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize()}};
  const std::vector<std::string> docs = {"Dr. Laurel writes APIs. Second sentence!", "We keep examples, e.g. docs for APIs.\nFourth line?"};
  const nlp::WorkflowReport report = wf.assess(docs, nlp::AssessOptions{.require_sentences = true});

  std::cout << "docs=" << report.doc_count << " tokens=" << report.token_count << "\n";
  std::cout << "sentences=" << report.sentence_count << "\n";
  std::cout << "mean_tokens=" << report.mean_tokens_per_doc << " mean_sentences=" << report.mean_sentences_per_doc << "\n";
  return 0;
}
```

Output

```text
docs=2 tokens=20
sentences=4
mean_tokens=10 mean_sentences=2
```

### Example 2

Without a sentencizer, the sentence fields stay at zero.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const std::vector<std::string> docs = {"Laurel writes APIs. Second sentence here."};
  const nlp::WorkflowReport report = wf.assess(docs);

  std::cout << "sentences=" << report.sentence_count << "\n";
  std::cout << "mean_sentences=" << report.mean_sentences_per_doc << "\n";
  return 0;
}
```

Output

```text
sentences=0
mean_sentences=0
```

## Changelog

- **0.1** -- initial documented surface.
