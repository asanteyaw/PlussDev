# Task Composition

Task stages turn a raw `Doc` into progressively richer annotations. They share one
in-memory document rather than translating between task-specific result objects, so
stage order is part of the program's meaning. Each stage writes to its own fields and
reads the fields that earlier stages produced. Select only the layers an application
needs, and place each consumer after the stage that produces its inputs.

## Data Dependencies

| Stage | Reads | Writes |
| --- | --- | --- |
| `RuleTokenizerStage` | `Doc::text`, `Doc::lang` | tokens, offsets, lexicon, morphology |
| `SentencizerStage` | tokens and their character offsets | sentence spans and boundary flags |
| `LemmatizerStage` | tokens | token lemmas and lemma ids |
| `MatcherStage` | tokens; lemmas when patterns use them | labeled match spans |
| `TaggerStage` | tokens; lemmas when available | token part-of-speech tags |
| `SyntaxStage` | tokens; tags, and sentence spans when present | heads, roles, and phrases |
| `NerStage` | tokens; lemmas when patterns use them | entity spans and token entity labels |
| `TextCatStage` | tokens, or the document text when no feature remains | document label, score, or label set |

A pipeline that uses every layer looks like this. Each stage is optional, and the
order is the one the table implies.

Run the full stage order on one sentence, with every stage in the pipeline present. Untrained tagger, parser, entity, and text-classification stages are included but do not change the counts printed here.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow pipeline{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::sentencize(),
          nlp::stage::lemmatize(),
          nlp::stage::matcher(),
          nlp::stage::tagger(),
          nlp::stage::syntax(),
          nlp::stage::ner(),
          nlp::stage::textcat(),
      },
  };
  const nlp::Doc doc = pipeline.infer("Ada writes books in London.");
  std::cout << "tokens = " << doc.tokens.size() << "\n";
  std::cout << "sentences = " << doc.sentence_texts().size() << "\n";
  return 0;
}
```

Output

```text
tokens = 6
sentences = 1
```

That order is not mandatory. A phrase-only pipeline may stop after tokenization and
matching, and a classifier may need only tokenization, lemmatization, and text
classification. Omitting unused stages reduces both work and state.

## Replacement Versus Enrichment

A stage owns its output layer. Running the tokenizer again rebuilds the lexical
foundation and clears every downstream annotation. Running the sentencizer, matcher,
tagger, syntax, NER, or classifier again clears and recomputes that stage's own output,
so repeated inference never accumulates stale spans. Two stages that write the same
field are therefore alternatives, not partners, unless their documented contract says
they combine.

NER is the notable place where several sources combine. Rules, learned phrases,
heuristics, and sequence predictions all feed one entity layer under a fixed precedence
and overlap policy. A matcher behaves differently: its patterns all contribute to one
match list, and a second matcher stage in the same pipeline starts by clearing the
first one's matches.

## Rule And Learned State

The tokenizer, sentencizer, lemmatizer, matcher, and syntax stages are rule-based. They
need no training and keep no learned state. The tagger and NER can run on their
heuristics before training, and they can also learn from annotated documents. The text
classifier emits no label until it has been trained. Each stage page describes how to
switch off its fallback paths when you want to measure learned behavior in isolation.

Factories return a `Stage`, which is a shared handle to the stage. Copying a workflow, or
placing one handle in several workflows, shares the stage's learned state. That is useful
when one model serves several front ends, but it is not a copy. Build separate stages, or
save and load a workflow, when the state must be independent.

## Persistence And Assessment

Every built-in task has a stable registry name and a state-line serializer.
`Workflow::save()` and `Workflow::load()` are the public way to persist a pipeline. The
per-stage state methods mainly support registries and custom containers.

Use [`assess::tagger`](assess_tagger.md) for aligned part-of-speech accuracy and
[`assess::ner`](assess_ner.md) for exact labeled-span precision, recall, and F1.
[`Workflow::assess`](../workflow/Workflow/assess.md) answers a different question: it
reports how much text was processed and how it was split, and it needs no gold reference.

## Practical Examples

These examples run a full pipeline and show what each layer contributes, and then show
that re-running a stage replaces its output rather than adding to it.

### Example 1

A pipeline with every built-in layer and no training. The tagger uses its heuristics,
the entity layer uses its rules and heuristics, and the classifier, which has never been
trained, emits no label. The program prints one line per layer, so each stage's
contribution can be read from the output. The first role is `root` rather than the
subject's `nsubj`, and that is a limit of the heuristics, not a fault in the syntax
stage: `ships` is tagged `NOUN` because `ship` is not in the heuristic verb list, so
the syntax stage has no verb to make the root and falls back to the first noun.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow pipeline{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::sentencize(),
          nlp::stage::lemmatize(),
          nlp::stage::matcher({{.label = "ACTION", .tokens = {"ship"}}}),
          nlp::stage::tagger(),
          nlp::stage::syntax(),
          nlp::stage::ner(),
          nlp::stage::textcat(),
      },
  };
  const nlp::Doc doc = pipeline.infer("Ada ships the APIs from New York. Kofi reads books.");

  std::cout << "tokens = " << doc.tokens.size() << "\n";
  std::cout << "sentences = " << doc.sentences.size() << "\n";
  std::cout << "matches = " << doc.matches.size() << "\n";
  std::cout << "phrases = " << doc.phrases.size() << "\n";
  std::cout << "entities = " << doc.entities.size() << "\n";
  std::cout << "doc_label = \"" << doc.doc_label << "\"\n";
  std::cout << "first tag = " << doc.tokens.front().pos << ", first role = " << doc.tokens.front().role << "\n";
  return 0;
}
```

Output

```text
tokens = 12
sentences = 2
matches = 1
phrases = 6
entities = 1
doc_label = ""
first tag = PROPN, first role = root
```

### Example 2

Replacement, not accumulation. The first part runs a matcher once, then runs two matcher
stages in a row, and the match count stays at two. The second part applies the tokenizer
to a document that already has tokens, and the stream is rebuilt from the text, so the
token count reflects one pass.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow once{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(),
                                nlp::stage::matcher({{.label = "PHRASE", .tokens = {"new", "york"}}})}};
  nlp::Doc doc = once.infer("New York and New York.");
  std::cout << "after one matcher pass: matches = " << doc.matches.size() << "\n";

  nlp::Workflow again{.lang = nlp::Language::English,
                      .stages = {nlp::stage::tokenize(),
                                 nlp::stage::matcher({{.label = "PHRASE", .tokens = {"new", "york"}}}),
                                 nlp::stage::matcher({{.label = "PHRASE", .tokens = {"new", "york"}}})}};
  doc = again.infer("New York and New York.");
  std::cout << "after two matcher stages: matches = " << doc.matches.size() << "\n";

  nlp::RuleTokenizerStage tokenizer{};
  doc = once.infer("A short text.");
  tokenizer.apply(doc);
  std::cout << "tokens after re-tokenizing = " << doc.tokens.size() << "\n";
  return 0;
}
```

Output

```text
after one matcher pass: matches = 2
after two matcher stages: matches = 2
tokens after re-tokenizing = 4
```

## See Also

- [`Workflow`](../workflow/Workflow.md) -- composes stages and runs them over documents.
- [`IStage`](../core/IStage.md) -- the interface every stage implements.
- [`RuleTokenizerStage`](RuleTokenizerStage.md) -- the first stage in almost every pipeline.
- [`assess::tagger`](assess_tagger.md) and [`assess::ner`](assess_ner.md) -- score learned output against gold references.

## Changelog

- **0.1** -- initial documented surface.
