# `pluss::ai::procedure` (nlp)

`nlp` owns the one way to wrap an [`nlp`](../plussai_nlp.md) `Workflow`
as a [`Step`](core/Step.md).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](nlp/step.md) | wraps an `nlp::Workflow` as a persistable learn/infer/assess step |

## How To Call It

Run an nlp workflow as a procedure step. The step returns one document per input text, and each document carries its own sentences.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>
#include <pluss/ai/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()},
  };

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("nlp", "text", "docs", wf),
  });

  const std::vector<std::string> texts = {"Ada writes code. Kofi reads books.", "Pluss runs locally."};
  const ai::ProcedureResult out = proc.run({ai::procedure::input("text", texts)});
  const auto& docs = out.get<std::vector<nlp::Doc>>("docs");
  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "documents = " << docs.size() << "\n";
  std::cout << "sentences in first document = " << docs[0].sentences.size() << "\n";
  return 0;
}
```

Output

```text
ok = true
documents = 2
sentences in first document = 2
```

The step accepts whichever input shape the active
[`RunOptions::mode`](core/RunOptions.md) and content call for -- a text
vector, a `pluss::Note`, a `std::vector<nlp::Doc>` (for `Learn`/`Assess`),
or a single `std::string` (for the default `Infer`) -- mirroring
`nlp::Workflow`'s own overloaded methods directly.

## Mode And Artifact Contract

| Mode | Accepted input | Published output |
| --- | --- | --- |
| `Infer` | `std::string`, `std::vector<std::string>`, or `Note` | `std::vector<nlp::Doc>` |
| `Learn` | `std::vector<std::string>`, `Note`, or `std::vector<nlp::Doc>` | Trained `nlp::Workflow` |
| `Assess` | `std::vector<std::string>`, `Note`, or `std::vector<nlp::Doc>` | `nlp::WorkflowReport` |

A scalar string is inference-only because it cannot describe a training
corpus. A `Note` is read through its text lanes without changing its shape or
dtype; the workflow's own document output becomes the artifact crossing the
next procedure edge.

## Ownership And Persistence

The workflow and option objects are moved into state owned by the step. A
learning run mutates that state for later inference through the same
procedure. The adapter is persistable without a caller-supplied registry key
because the NLP library provides its own stable workflow save/load contract. Bundle
metadata records the relevant training and assessment options alongside the
component artifact.

## Choosing The Boundary

Use one NLP step for one coherent workflow, not one procedure step per internal
tokenizer/tagger stage. The workflow already owns ordering and shared document
state between those stages. Procedure should coordinate that workflow with
tabular, numeric, model, or sink steps at the larger deployment-graph level.

## Related Pages

- [`nlp`](../plussai_nlp.md) -- the framework `Workflow` belongs to.
- [`lymph`](lymph.md) / [`ice`](ice.md) -- the sibling adapters for
  other engines.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

nlp/Introduction
nlp/step
```
<!-- apiref:toctree:end -->
