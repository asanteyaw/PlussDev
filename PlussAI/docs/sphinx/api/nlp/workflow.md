# `pluss::ai::nlp` (workflow)

`workflow` owns the pipeline object every other `nlp` category feeds into.
[`Workflow`](workflow/Workflow.md) is an ordered list of stages plus the
calls that run them: `learn` to train the trainable stages, `infer` to turn
raw text into annotated documents, `assess` to summarize a corpus, and
`save`/`load` to persist the whole pipeline. [`WorkflowReport`](workflow/WorkflowReport.md)
is what `assess` returns.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Workflow`](workflow/Workflow.md) | the pipeline: a stage list, plus `append_stage`/`learn`/`infer`/`assess`/`save`/`load` |
| [`WorkflowReport`](workflow/WorkflowReport.md) | corpus-level counts and per-document means from `assess` |

## How To Call It

Run a three-stage nlp workflow on one text. The program prints the token count, the sentence count, and each token with its lemma. Lemmatization lowercases the fallback form, so `Dr.` becomes `dr.`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize()},
  };
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs. Second sentence!");

  std::cout << "tokens = " << doc.tokens.size() << "\n";
  std::cout << "sentences = " << doc.sentence_texts().size() << "\n";
  for (const auto& token : doc.tokens) {
    std::cout << "  " << token.orth << " -> " << token.lemma << "\n";
  }
  return 0;
}
```

Output

```text
tokens = 8
sentences = 2
  Dr. -> dr.
  Laurel -> laurel
  writes -> write
  APIs -> api
  . -> .
  Second -> second
  sentence -> sentence
  ! -> !
```

Every pipeline follows the same shape: build the stage list once, `learn`
from labeled or raw text only if a stage needs training, then `infer` over
new text. A pipeline with no trainable stage can skip `learn` entirely.

## Related Pages

- [`core`](core.md) -- the `Doc` every call returns.
- [`tasks`](tasks.md) -- the stages a pipeline is built from.
- [`io`](io.md) -- the binary format `save` and `load` use.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

workflow/Introduction
workflow/Workflow
workflow/WorkflowReport
```
<!-- apiref:toctree:end -->
