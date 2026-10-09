# `pluss::ai::nlp` Tasks

Tasks are the ready-made `IStage` implementations placed inside a
[`Workflow`](workflow/Workflow.md). They cover text segmentation, lexical and
sentence enrichment, deterministic span rules, learned token annotation,
syntax, entity recognition, and document classification. Factory functions in
`pluss::ai::nlp::stage` return the shared `Stage` handle expected by a
workflow; concrete classes remain available when a caller needs `learn()`,
`add_pattern()`, or direct state ownership.

Read [Task Composition](tasks/Introduction.md) first when choosing stage order
or deciding which layers can be omitted.

## Stage Catalog

| Factory | Stage | Reads | Produces |
| --- | --- | --- | --- |
| `stage::tokenize()` | [`RuleTokenizerStage`](tasks/RuleTokenizerStage.md) | `Doc::text`, language | tokens and lexical records |
| `stage::sentencize()` | [`SentencizerStage`](tasks/SentencizerStage.md) | tokens, language | sentence flags and spans |
| `stage::lemmatize()` | [`LemmatizerStage`](tasks/LemmatizerStage.md) | tokens, language | token lemmas |
| `stage::matcher()` | [`MatcherStage`](tasks/MatcherStage.md) | token surfaces or lemmas | labeled match spans |
| `stage::tagger()` | [`TaggerStage`](tasks/TaggerStage.md) | tokens or lemmas | part-of-speech labels |
| `stage::syntax()` | [`SyntaxStage`](tasks/SyntaxStage.md) | tokens and preferably tags | heads, roles, and phrases |
| `stage::ner()` | [`NerStage`](tasks/NerStage.md) | token surfaces plus optional learned state | entity spans and labels |
| `stage::textcat()` | [`TextCatStage`](tasks/TextCatStage.md) | prepared document tokens | document labels and scores |

Pattern stages are deterministic and configured at construction. Tagging and entity
recognition run a fixed sequence of sources, and each page states the precedence of
its sources. Text classification has a classic backend for single-label output and a
sequence backend that also supports multi-label output. Each stage page documents its
options field by field, the order of its operations, its validation failures, its
persistence behavior, and its verified programs.

## Accuracy Reports

[`assess::tagger`](tasks/assess_tagger.md) compares the tags of each token with the gold
tags, after checking that the predicted and gold documents align token for token.
[`assess::ner`](tasks/assess_ner.md) matches predicted entities to gold entities one to
one, on token range and label, and reports precision, recall, and F1. The
[`TagScore`](tasks/TagScore.md) and [`EntityScore`](tasks/EntityScore.md) structs hold the
counts behind those figures, and their pages show how to combine counts from separate
corpora. These are task metrics; [`Workflow::assess`](workflow/Workflow/assess.md) instead
reports how much text a pipeline processed.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

tasks/Introduction
tasks/assess_ner
tasks/assess_tagger
tasks/EntityPattern
tasks/EntityScore
tasks/LemmatizerStage
tasks/MatcherPattern
tasks/MatcherStage
tasks/NerStage
tasks/RuleTokenizerStage
tasks/SentencizerStage
tasks/SyntaxStage
tasks/TaggerStage
tasks/TagScore
tasks/TextCatStage
tasks/TrainingReport
```
<!-- apiref:toctree:end -->
