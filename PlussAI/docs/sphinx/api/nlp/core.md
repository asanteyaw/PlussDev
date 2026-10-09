# `pluss::ai::nlp` (core)

`core` owns every type the rest of `nlp` is built from:
[`Doc`](core/Doc.md) (one processed document), its annotation units
([`Token`](core/Token.md)/[`Span`](core/Span.md)), the type-level
lexical state it accumulates
([`Lexicon`](core/Lexicon.md)/[`Lexeme`](core/Lexeme.md)/
[`TextPool`](core/TextPool.md)/[`MorphBank`](core/MorphBank.md)/
[`Morphology`](core/Morphology.md)), and [`IStage`](core/IStage.md), the
contract every pipeline stage implements.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Doc`](core/Doc.md) | one processed document: text, tokens, and every annotation layer a pipeline added |
| [`Span`](core/Span.md) | a labeled token range -- the shared shape behind sentences, phrases, matches, and entities |
| [`Token`](core/Token.md) | one token occurrence's full per-position annotation |
| [`Morphology`](core/Morphology.md) | a cached shape/casing/character-class analysis |
| [`Lexeme`](core/Lexeme.md) | the type-level (not per-occurrence) record for one distinct surface form |
| [`TextPool`](core/TextPool.md) | the deterministic string-interning table behind `StringId` |
| [`MorphBank`](core/MorphBank.md) | the `Morphology`-side counterpart to `TextPool` |
| [`Lexicon`](core/Lexicon.md) | per-`Doc` lexical state: interning plus one `Lexeme` per observed surface form |
| [`IStage`](core/IStage.md) | the contract every pipeline stage implements |

## How To Call It

Split a text into sentences with a tokenize and sentencize workflow, then print each sentence.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()},
  };
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs. Second sentence here.");

  int index = 0;
  for (const std::string& sentence : doc.sentence_texts()) {
    std::cout << "sentence " << ++index << ": " << sentence << "\n";
  }
  return 0;
}
```

Output

```text
sentence 1: Dr. Laurel writes APIs.
sentence 2: Second sentence here.
```

Nothing in `core` is usually constructed directly -- a [`workflow`](workflow.md)
`Workflow` built from [`tasks`](tasks.md) stages produces every `Doc` a
caller sees, and that `Doc`'s own `lexicon` is populated automatically
by whichever stages ran. `core`'s own types are what a caller reads
*from* a `Doc` afterward: token/sentence/entity text, morphology flags,
or (once a syntax-producing stage has run) dependency structure via
`Doc::children_of`/`lineage_of`/`branch_of`.

## Related Pages

- [`workflow`](workflow.md) -- `Workflow`, what actually produces a
  `Doc`.
- [`tasks`](tasks.md) -- the built-in `IStage` catalog.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

core/Introduction
core/Doc
core/Span
core/Token
core/Morphology
core/Lexeme
core/TextPool
core/MorphBank
core/Lexicon
core/IStage
```
<!-- apiref:toctree:end -->
