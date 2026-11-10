# `pluss::ai::nlp::EntityPattern`

`EntityPattern` is one deterministic entity rule for
[`NerStage`](NerStage.md). It names a label and an ordered sequence of token
strings, and it says whether the strings are compared against each token's
lemma or its surface form. It is plain data, a small aggregate built with
designated initializers, and it carries no behavior. The stage validates it
when the stage is constructed or when a pattern is added.

An entity pattern works like a [`MatcherPattern`](MatcherPattern.md) with a
different output. Each match becomes an entity span in `Doc::entities`, and
the pattern's label becomes the span's label and the `entity_label` of each
token it covers. Rules run before every other entity source, so a pattern
claims its tokens first, and a learned or heuristic entity cannot take them
when overlap is off. That priority is the main reason to write a rule instead
of relying on training.

## Declaration

### `pluss::ai::nlp::EntityPattern` fields

| Field | Type | Default |
|---|---|---|
| `label` | `std::string` | empty |
| `tokens` | `std::vector<std::string>` | empty |
| `use_lemmas` | `bool` | `true` |

## Fields

### `label`

`std::string`, empty by default, and an empty value is rejected. The label is
the entity type the pattern assigns, such as `GPE`, `PERSON`, or `PRODUCT`. It
is copied into each entity span the pattern produces and into the
`entity_label` of each covered token.

Labels are free-form. The stage does not know which labels are standard, so
it is the application's job to keep them consistent across rules and training
data. A rule and a learned phrase that use different spellings for the same
type, such as `GPE` and `Gpe`, produce two separate types in the output.

### `tokens`

`std::vector<std::string>`, empty by default, and it must contain at least one
string, none of them empty. Each string is one token of the entity, in order.
A two-word place name is `{"New", "York"}`, and a one-token name is a list with
one element.

The strings are compared with the document's tokens after normalization by the
stage's `lowercase` option, and they are not analyzed. A pattern written in
surface form matches only that spelling, and a pattern written in lemma form
needs `use_lemmas` on and a lemmatizer upstream. The rules for these choices are
the same as for `MatcherPattern`.

### `use_lemmas`

`bool`, `true` by default. It selects the token form compared against the
pattern. When `true`, the token's lemma is used if it is non-empty, and its
surface form otherwise. When `false`, the surface form is always used.

Set it to `false` for entities whose exact spelling is the signal: a
product name, an acronym, or a place name where an inflected form would be a
different entity. Keep it `true` for entities whose base form is stable across
inflection, such as a role or a title that appears in several grammatical forms.

## Guidance

Write a rule when the entity is a fixed phrase that the stage must never miss,
or never mislabel, and when a reviewer should be able to read why the label was
assigned. Rules are precise and brittle: they find exactly the phrases they name
and nothing else, so a new spelling needs a new pattern.

Because rules run first, a broad rule can override a precise learned label on
the same tokens. Keep rules narrow, and give them the labels that the learned
data would otherwise have to override.

## See Also

- [`NerStage`](NerStage.md) -- the stage that applies entity patterns.
- [`MatcherPattern`](MatcherPattern.md) -- the same shape for token-match rules that produce match spans.
- [`Span`](../core/Span.md) -- the entity record each pattern produces.
- [`LemmatizerStage`](LemmatizerStage.md) -- supplies the lemmas `use_lemmas` reads.

## Practical Examples

These examples build entity patterns, run them through a workflow, and print the
entities they produce. The first shows a rule that claims a phrase before the
heuristics can label it. The second shows the case flag and the lemma flag on the
same text. The third shows that a pattern with an unknown spelling does nothing.

### Example 1

A rule for a two-word place name, run on text where the heuristics would label
the same words as a person. The rule runs first, so its label wins and the
heuristic label is dropped.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                              nlp::stage::ner({{.label = "GPE", .tokens = {"New", "York"}, .use_lemmas = false}})}};
  const nlp::Doc doc = wf.infer("Mina moved to New York in May.");

  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ": \"" << entity.text(doc) << "\" tokens [" << entity.token_start
              << ", " << entity.token_end << ")\n";
  }
  return 0;
}
```

Output

```text
GPE: "New York" tokens [3, 5)
```

### Example 2

The `lowercase` and `use_lemmas` flags on the same text. The pattern `Tema`
matches `tema` only when the stage lowercases both sides, and the pattern
`writer` with lemmas on matches `writers` and `writer` through their common
lemma.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "tema writers and Tema writer";
  for (const bool lowercase : {true, false}) {
    nlp::NerOptions options{.lowercase = lowercase, .use_heuristics = false};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                                nlp::stage::ner({{.label = "GPE", .tokens = {"Tema"}, .use_lemmas = false},
                                                 {.label = "ROLE", .tokens = {"writer"}, .use_lemmas = true}},
                                                options)}};
    const nlp::Doc doc = wf.infer(text);

    std::cout << "lowercase = " << (lowercase ? "true" : "false") << "\n";
    for (const nlp::Span& entity : doc.entities) {
      std::cout << "  " << entity.label << ": " << entity.text(doc) << "\n";
    }
  }
  return 0;
}
```

Output

```text
lowercase = true
  GPE: tema
  ROLE: writers
  GPE: Tema
  ROLE: writer
lowercase = false
  ROLE: writers
  GPE: Tema
  ROLE: writer
```

### Example 3

A pattern whose spelling never occurs in the text produces no entities, and the
stage reports nothing rather than failing. The second rule matches, which shows
the two rules are independent.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::ner({
                       {.label = "PRODUCT", .tokens = {"Quantum", "Lattice"}, .use_lemmas = false},
                       {.label = "GPE", .tokens = {"Accra"}, .use_lemmas = false},
                   }, nlp::NerOptions{.use_heuristics = false})}};
  const nlp::Doc doc = wf.infer("Ada flew to Accra.");

  std::cout << "entities = " << doc.entities.size() << "\n";
  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ": " << entity.text(doc) << "\n";
  }
  return 0;
}
```

Output

```text
entities = 1
GPE: Accra
```

## Changelog

- **0.1** -- initial documented surface.
