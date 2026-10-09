# `pluss::ai::nlp::NerStage`

`NerStage` recognizes labeled entity spans and writes them to `Doc::entities`.
Each entity is a token range with a label, such as `PERSON`, `GPE`, or `ORG`,
and each covered token also gets that label in its `entity_label` field.

Entities come from four sources, and the stage consults them in a fixed order.
Explicit [`EntityPattern`](EntityPattern.md) rules run first. Learned phrase
tables run next, when `prefer_learned` is on and the stage has been trained.
Built-in heuristics run after that, covering e-mail addresses, URLs, uppercase
organization names, and title-led or capitalized name runs. The contextual
sequence classifier runs last, when the Sequence backend is trained. Each source
can add entities, and the `allow_overlap` option decides what happens when two
of them claim the same tokens.

Entity recognition is a sequence of decisions, and the order is the policy. A
rule that names a place claims its tokens before a heuristic can label them as a
person, and a learned phrase claims its tokens before the heuristics run. Read
the order before you add a source, because the first source to claim a token
usually decides its label.

## Signature

```cpp
pluss::ai::nlp::NerStage::NerStage(
    std::vector<pluss::ai::nlp::EntityPattern> patterns = {},
    pluss::ai::nlp::NerOptions options = {});
pluss::ai::nlp::NerStage::NerStage(
    std::initializer_list<pluss::ai::nlp::EntityPattern> patterns,
    pluss::ai::nlp::NerOptions options = {});
pluss::ai::nlp::NerStage& pluss::ai::nlp::NerStage::add_pattern(
    pluss::ai::nlp::EntityPattern pattern);
void pluss::ai::nlp::NerStage::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    const pluss::ai::nlp::TrainOptions& options);
pluss::ai::nlp::Stage pluss::ai::nlp::stage::ner(
    std::vector<pluss::ai::nlp::EntityPattern> patterns = {},
    pluss::ai::nlp::NerOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::ner(
    std::initializer_list<pluss::ai::nlp::EntityPattern> patterns,
    pluss::ai::nlp::NerOptions options = {});
```

## Construction

Use `stage::ner` in a workflow. Keep the concrete class when you need to call
`learn` or `add_pattern`, because a `Stage` exposes only the interface that
every stage shares.

Both constructors validate every pattern before the stage is returned. A
pattern with an empty label, an empty token list, or an empty token string
throws. A stage with no patterns is valid: it then relies on learned state and
heuristics alone.

## Options

The options struct is `pluss::ai::nlp::NerOptions`. Its defaults give heuristic
entities, learned phrases when available, no overlap, and the classic backend.

### `lowercase`

Controls whether pattern tokens and learned phrases compare lowercase forms.
When `true`, `Tema` and `tema` are the same key in both places. When `false`,
the comparison is exact.

The title check in the heuristics always lowercases, whatever this option says,
so `Dr.` is recognized as a title either way.

### `allow_overlap`

Controls whether two entities may cover the same token. The default is `false`,
which is the safer choice for most output.

When `false`, a candidate is rejected if it intersects any entity already
accepted, including one from an earlier source. The first source to claim a
token keeps it, so rules win over learned phrases and learned phrases win over
heuristics. When `true`, every candidate is kept. Overlapping spans then both
appear in `Doc::entities`, but each token's `entity_label` still comes from the
first entity that covers it.

Turn overlap on when a nested reading matters, such as an organization inside
a longer phrase that is itself a title. Leave it off when each token should have
one meaning, which is what most downstream code assumes.

### `use_heuristics`

Controls whether the built-in heuristics run. They are applied after the rules and
the learned phrases, and they follow these rules, checked in this order for each
token:

- a token that looks like an e-mail address is `EMAIL`;
- a token that looks like a URL is `URL`;
- an uppercase token of at least two characters is `ORG`;
- a title such as `Dr.`, `Mr.`, `Mrs.`, `Ms.`, `Prof.`, or `Sir`, followed by
  one or more capitalized tokens, forms a `PERSON` entity that starts at the title;
- any run of two or more capitalized tokens forms a `PERSON` entity.

The last rule is broad. A run of two capitalized words is labeled `PERSON`
whatever it is, so a place name such as `New York` gets that label unless a rule
or learned phrase claimed it first. A single capitalized word is not labeled by
the heuristics at all.

Turn this off for a corpus where the rules are the only source of truth, or
when the heuristic labels would be wrong more often than right.

### `prefer_learned`

Controls whether the learned phrase tables are applied. When `true` and the stage
has been trained, every phrase in the tables is looked up in the document, longest
phrases first, and each match is claimed before the heuristics run. When `false`,
the tables are ignored, even when they were trained.

Turn it off to see what the rules and heuristics alone would produce, or to
compare a learned run against a rules-only run on the same corpus.

### `backend`

Selects the trainable component. `TaskBackend::Classic` uses the phrase tables
only. `TaskBackend::Sequence` also trains a contextual classifier that labels each
token with a begin-inside-outside tag, and its predictions are decoded into spans
after the other sources have run.

### `sequence_width`

`std::int64_t`, default `48`. The hidden width of the Sequence backend's
classifier. A larger width has more capacity and costs more to train. It has no
effect on the Classic backend.

### `sequence_max_span_width`

`std::int64_t`, default `4`. The longest entity the decoder will produce from the
classifier's predictions. A tag run longer than this is cut at the limit, so
the remainder becomes a new span only if the classifier starts it again. Raise it
for corpora with long names, and lower it to keep the decoder from merging
neighboring entities.

### `sequence_min_score`

`double`, default `0.55`. The lowest probability that the classifier must assign to
every token of a decoded span for the span to be kept. A span that dips below the
threshold on any one token is dropped. Lower it to recall more entities at the
cost of precision, and raise it when a missed entity is cheaper than a wrong one.

### `sequence_seed`

`std::uint64_t`, default `0`. The seed for the classifier's initial weights. Two
runs with the same seed, the same training documents, and the same width produce
the same predictions.

## State

A trained stage keeps its learned phrase tables and, with the Sequence backend, the
classifier's weights. It also keeps its patterns and options.

`dump_state_lines()` writes the options, then the patterns, then the learned tables,
then the classifier state. Labels and tokens are hex-encoded, so a saved file
carries any text safely. `load_state_lines()` restores a complete state and
validates it. A file whose pattern count or table size does not match is rejected.

## Methods

### `name()`

Returns `"ner"`.

### `add_pattern(pattern)`

Validates the pattern, appends it, and returns a reference to the stage so calls can
be chained. The new pattern is tried after the patterns already in the list, so an
earlier rule keeps a contested token. A pattern that fails validation throws, and the
stage is left unchanged.

### `learn(docs, options)`

Trains from the entities already on the documents. Each gold entity contributes
its phrase, normalized by the `lowercase` option, and its label. The phrase tables
are keyed by phrase width, and each phrase keeps its most frequent label, with a
lexicographic tie-break, so training is deterministic. Documents with no entities
contribute nothing to the tables.

With the Sequence backend, the gold entities are also converted into per-token
begin-inside-outside tags, and the classifier trains on them. Every token outside
an entity is tagged `O`. The classifier uses lemmas when a token has them, and it
lowercases tokens according to the `lowercase` option. It needs at least two
distinct tags, so a corpus with no entities at all, where every token is `O`,
throws.

Calling `learn` replaces the previous training. `TrainOptions` is accepted for
interface consistency, and the classifier's randomness comes from `sequence_seed`.

### `apply(Doc&)`

Clears `Doc::entities` and every token's `entity_label`, then assigns entities from
the sources in the order described at the top of this page. The result is sorted by
start, then end, then label. Finally, each token that an entity covers receives that
entity's label, unless an earlier entity already labeled it.

### `dump_state_lines()`

Returns the options, the patterns, and the learned state.

### `load_state_lines(lines)`

Replaces the complete state from a saved set of lines and validates it.

### `training_report()`

Returns the [`TrainingReport`](TrainingReport.md) of the most recent `learn` call: the
number of training documents, the number of learned phrases across all widths, and, for
the Sequence backend, the loss after each optimizer step. An untrained stage returns an
empty record.

## Practical Examples

These examples show the default entities on a realistic sentence, the order of
the sources and the overlap rule, learned phrases applied to a new document, and
the Sequence backend with its training requirement.

### Example 1

The default entity recognizer on a sentence with an e-mail address, an
organization, and a two-word place name. The e-mail and the organization come
from the heuristics. The place name is labeled as a person, because the heuristics
label any run of capitalized words that way. The single capitalized name `Kofi`
is not an entity at all.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::ner()}};
  const nlp::Doc doc = wf.infer("Kofi emailed help@pluss.dev from NASA in New York.");

  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ": \"" << entity.text(doc) << "\" tokens [" << entity.token_start
              << ", " << entity.token_end << ")\n";
  }
  return 0;
}
```

Output

```text
EMAIL: "help@pluss.dev" tokens [2, 3)
ORG: "NASA" tokens [4, 5)
PERSON: "New York" tokens [6, 8)
```

### Example 2

The source order and the overlap rule. A rule labels `New York` as a place, and
the heuristics would label the same tokens as a person. With overlap off, the rule
keeps the tokens. With overlap on, both entities appear, but each token's
`entity_label` still comes from the first one.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool overlap : {false, true}) {
    nlp::NerOptions options{.allow_overlap = overlap};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                                nlp::stage::ner({{.label = "GPE", .tokens = {"New", "York"}, .use_lemmas = false}},
                                                options)}};
    const nlp::Doc doc = wf.infer("Mina moved to New York.");

    std::cout << "allow_overlap = " << (overlap ? "true" : "false") << "\n";
    for (const nlp::Span& entity : doc.entities) {
      std::cout << "  " << entity.label << ": " << entity.text(doc) << "\n";
    }
    std::cout << "  entity_label of \"York\" = " << doc.tokens[4].entity_label << "\n";
  }
  return 0;
}
```

Output

```text
allow_overlap = false
  GPE: New York
  entity_label of "York" = GPE
allow_overlap = true
  GPE: New York
  PERSON: New York
  entity_label of "York" = GPE
```

### Example 3

Learned phrases applied to a new document. The classifier is not used here,
because the backend is classic. The tables learned from two gold-labeled
documents label the phrase `Accra` in a document that has no rule for it. The
heuristics are off, so the label can only come from training.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::Doc first = base.infer("Kofi visits Tema.");
  first.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}, {.token_start = 2, .token_end = 3, .label = "GPE"}};
  nlp::Doc second = base.infer("Ada visits Accra.");
  second.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}, {.token_start = 2, .token_end = 3, .label = "GPE"}};

  auto ner = std::make_shared<nlp::NerStage>(std::vector<nlp::EntityPattern>{}, nlp::NerOptions{.use_heuristics = false});
  ner->learn({first, second}, {});
  const nlp::TrainingReport report = ner->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), ner}};
  const nlp::Doc doc = wf.infer("Kofi visits Accra.");
  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ": " << entity.text(doc) << "\n";
  }
  return 0;
}
```

Output

```text
docs = 2, entries = 4, steps = 0
PERSON: Kofi
GPE: Accra
```

### Example 4

The Sequence backend on the same gold-labeled documents, applied to an unseen
sentence with the heuristics off. The classifier produces the entities, and the
decoder keeps them as spans. The output is seeded, so it is the same on every run.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::Doc first = base.infer("Kofi visits Tema.");
  first.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}, {.token_start = 2, .token_end = 3, .label = "GPE"}};
  nlp::Doc second = base.infer("Ada visits Accra.");
  second.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}, {.token_start = 2, .token_end = 3, .label = "GPE"}};

  auto ner = std::make_shared<nlp::NerStage>(
      std::vector<nlp::EntityPattern>{},
      nlp::NerOptions{.prefer_learned = false,
                      .use_heuristics = false,
                      .backend = nlp::TaskBackend::Sequence,
                      .sequence_width = 16,
                      .sequence_min_score = 0.1,
                      .sequence_seed = 9});
  ner->learn({first, second}, {});
  const nlp::TrainingReport report = ner->training_report();
  std::cout << "docs = " << report.doc_count << ", entries = " << report.entry_count
            << ", steps = " << report.steps() << "\n";
  if (report.steps() > 0) std::cout << "final loss = " << report.loss_history.back() << "\n";

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), ner}};
  const nlp::Doc doc = wf.infer("Kofi visits Accra.");
  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ": " << entity.text(doc) << "\n";
  }
  return 0;
}
```

Output

```text
docs = 2, entries = 4, steps = 60
final loss = 0.0114379
PERSON: Kofi
GPE: Accra
```

### Example 5

Two errors that callers meet early. A pattern with an empty label is rejected when
it is added, and a Sequence backend trained on a corpus with no entities throws,
because every token is then the same outside tag. The program prints the first line
of each message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::NerStage ner{};
  try {
    ner.add_pattern({.label = "", .tokens = {"Accra"}});
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc plain = base.infer("Ada visits Accra.");
  nlp::NerStage trainable{std::vector<nlp::EntityPattern>{}, nlp::NerOptions{.backend = nlp::TaskBackend::Sequence}};
  try {
    trainable.learn({plain}, {});
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
nlp::NerStage: entity-pattern labels must not be empty.
nlp::detail::train_token_classifier: expected at least two distinct labels.
```

## See Also

- `stage::ner` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`EntityPattern`](EntityPattern.md) -- the rule record this stage applies first.
- [`TaggerStage`](TaggerStage.md) -- the part-of-speech counterpart, with the same backend choice.
- [`assess::ner`](assess_ner.md) -- scores entities against gold spans.
- [`Doc::entity_texts`](../core/Doc/entity_texts.md) -- returns the text of each entity.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
