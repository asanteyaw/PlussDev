<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 041ed4f14984 -->

# `pluss::ai::nlp::Workflow::append_stage`

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## Signature

```cpp
pluss::ai::nlp::Workflow& pluss::ai::nlp::Workflow::append_stage(pluss::ai::nlp::Stage stage);
```

## What It Does

Adds a stage to the end of the pipeline and returns the workflow, so calls can be chained. The new stage runs last, after every stage already in the list. It is the way to build a pipeline in code step by step rather than in one brace list.

## Parameters

`Stage stage`
: A shared pointer to an [`IStage`](../../core/IStage.md), typically a factory result from [`tasks`](../../tasks.md).

## Returns

A reference to this workflow, for chaining.

## Errors

None.

## Practical Examples

### Example 1

Appending a stage after construction adds it to the end of the list.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  wf.append_stage(nlp::stage::lemmatize());
  std::cout << "stages=" << wf.stages.size() << "\n";
  return 0;
}
```

Output

```text
stages=2
```

### Example 2

Calls chain, so a whole pipeline can be assembled in one expression.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  wf.append_stage(nlp::stage::sentencize()).append_stage(nlp::stage::lemmatize());
  const nlp::Doc doc = wf.infer("Laurel writes APIs. Second sentence.");
  std::cout << "sentences=" << doc.sentence_texts().size() << " lemmas=" << doc.lemma_texts().size() << "\n";
  return 0;
}
```

Output

```text
sentences=2 lemmas=7
```

### Example 3

A stage appended later runs after the earlier ones, so it sees their output.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  wf.append_stage(nlp::stage::lemmatize());
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");
  std::cout << "first_lemma=" << doc.tokens[0].lemma << "\n";
  return 0;
}
```

Output

```text
first_lemma=laurel
```

## See Also

- [`Workflow`](../Workflow.md) -- the pipeline this changes.
- [`tasks`](../../tasks.md) -- the factories that produce stages.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
