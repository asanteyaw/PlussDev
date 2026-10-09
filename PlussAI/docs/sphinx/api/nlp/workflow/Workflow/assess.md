<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c2446689ebce -->

# `pluss::ai::nlp::Workflow::assess`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
pluss::ai::nlp::WorkflowReport pluss::ai::nlp::Workflow::assess(pluss::RangeView<std::string_view> texts, const pluss::ai::nlp::AssessOptions& options = {}) const;
pluss::ai::nlp::WorkflowReport pluss::ai::nlp::Workflow::assess(const std::vector<std::string>& texts, const pluss::ai::nlp::AssessOptions& options = {}) const;
pluss::ai::nlp::WorkflowReport pluss::ai::nlp::Workflow::assess(const pluss::Note& texts, const pluss::ai::nlp::AssessOptions& options = {}) const;
pluss::ai::nlp::WorkflowReport pluss::ai::nlp::Workflow::assess(const std::vector<pluss::ai::nlp::Doc>& docs, const pluss::ai::nlp::AssessOptions& options = {}) const;
```

## What It Does

Runs the pipeline over a corpus and returns a [`WorkflowReport`](../WorkflowReport.md): document, token, and sentence counts, with per-document means. Text inputs are annotated first, as `infer` would; document inputs are summarized as they are, so pass documents you already produced to avoid running the pipeline twice. The options field `require_sentences` asks the assessment to expect sentence boundaries, which matters when a corpus is meant to have them.

## Parameters

`texts` / `docs`
: The corpus to assess, as strings, a `Note`, a range of views, or documents already produced by `infer`.

`options`
: An `AssessOptions`. `require_sentences` (default `false`) requests that sentence boundaries be present.

## Returns

A `WorkflowReport`.

## Errors

Propagates any error a stage throws while annotating a text input.

## Practical Examples

### Example 1

Assessing a list of texts reports the corpus totals.

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
  std::cout << "docs=" << report.doc_count << " sentences=" << report.sentence_count << "\n";
  return 0;
}
```

Output

```text
docs=2 sentences=4
```

### Example 2

Assessing documents that were already produced skips re-running the pipeline.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const std::vector<nlp::Doc> docs = wf.infer(std::vector<std::string>{"Laurel writes APIs. Second sentence!"});
  const nlp::WorkflowReport report = wf.assess(docs);
  std::cout << "docs=" << report.doc_count << " tokens=" << report.token_count << "\n";
  return 0;
}
```

Output

```text
docs=1 tokens=7
```

### Example 3

A `Note` of texts gives the same report as the equivalent list.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::WorkflowReport report = wf.assess(pluss::make_note({"Laurel writes APIs.", "Second text here."}));
  std::cout << "docs=" << report.doc_count << " tokens=" << report.token_count << "\n";
  return 0;
}
```

Output

```text
docs=2 tokens=8
```

## See Also

- [`WorkflowReport`](../WorkflowReport.md) -- every field of the result.
- [`infer`](infer.md) -- the per-document counterpart.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append_stage.html">
    <span class="label">Previous</span>
    <span class="title">append_stage</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
