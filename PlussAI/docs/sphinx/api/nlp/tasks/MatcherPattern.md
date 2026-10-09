# `pluss::ai::nlp::MatcherPattern`

`MatcherPattern` is one phrase rule for [`MatcherStage`](MatcherStage.md). It
names a label and an ordered sequence of token strings, and it says whether
those strings are compared against each token's lemma or its surface form. It
is plain data, a small aggregate that is built with designated initializers,
and it carries no behavior of its own. The stage validates it when the stage
is constructed or when a pattern is added, and it copies the pattern into its
own list.

A pattern matches a run of adjacent tokens whose normalized forms equal the
pattern's tokens in order. Each match becomes a `Span` in `Doc::matches`
whose label is the pattern's label. A pattern is deterministic: the same
document and the same pattern always give the same matches, and no training
is involved.

## Declaration

```cpp
struct pluss::ai::nlp::MatcherPattern {
  std::string label{};
  std::vector<std::string> tokens{};
  bool use_lemmas = true;
};
```

## Fields

### `label`

`std::string`, empty by default, and an empty value is rejected. The label is
the application's name for what the pattern finds, such as `CITY` or `ACTION`.
It is copied unchanged into every match span the pattern produces, so the
label is the key for reading results back: a caller who filters `Doc::matches`
by label gets exactly the spans one pattern found.

Two patterns may share a label. The stage does not check uniqueness, so two
rules with the label `ACTION` both feed the same bucket of spans. Use a shared
label when two surface forms mean the same thing, and separate labels when
they must be told apart downstream.

### `tokens`

`std::vector<std::string>`, empty by default, and it must contain at least one
string. No string in it may be empty. Each string is one token of the phrase,
in order. The phrase `New York` is `{"New", "York"}`, and a single-token
pattern is a one-element list.

Each pattern token is compared with the document token at the same offset in
the run, after both sides are normalized. The stage lowercases both sides when
its `lowercase` option is on, and it leaves them as written when it is off. The
pattern's own strings are not analyzed: they are not lemmatized, not split, and
not checked against the tokenizer. That is why a lemma-based pattern must be
written in lemma form. The Practical Examples show the failure that follows
from writing `wrote` for a token whose lemma is `write`.

### `use_lemmas`

`bool`, `true` by default. It selects which form of each document token is
compared. When `true`, a token's lemma is used if the lemma is non-empty, and
its surface form is used otherwise. When `false`, the surface form is always
used, and lemmas are ignored entirely.

Use `true` for patterns that should match every inflection of a word, provided
the pattern is written in lemma form. A pattern `{"write"}` then matches
`wrote`, `writes`, and `write`, because each has the lemma `write`. Use `false`
for patterns whose exact surface matters: a product name, a code, a title that
must stay capitalized, or a phrase whose meaning changes with inflection. This
option is also the right choice when no lemmatizer runs, since a token with no
lemma falls back to its surface form even when `use_lemmas` is `true`.

## Guidance

Choose a pattern when a rule is exact and explainable, and when the matches
will be reviewed by a person or used as hard evidence. Patterns are
brittle in one specific way: they match only the token strings they name, so
a spelling variant needs its own pattern unless the stage's `lowercase` option
and the lemmatizer already collapse it. For broad recognition over varied text,
a learned model usually covers more ground, and patterns are then best used to
pin down the cases the model gets wrong.

Keep patterns short and specific. A two-token pattern is far less likely to
fire by accident than a single common word, so a pattern for a common word
like `bank` should usually be paired with a neighboring token, or left to a
learned stage.

## See Also

- [`MatcherStage`](MatcherStage.md) -- the stage that applies patterns.
- [`EntityPattern`](EntityPattern.md) -- the same shape used by the entity recognizer.
- [`Span`](../core/Span.md) -- the match record each pattern produces.
- [`LemmatizerStage`](LemmatizerStage.md) -- supplies the lemmas `use_lemmas` reads.

## Practical Examples

These examples build patterns, run them through a workflow, and print the
matches they produce. The first shows the shape of a pattern and the spans it
returns. The second shows the lemma rule in both directions. The third shows
that a pattern is only as good as its token strings.

### Example 1

A two-token pattern and a single-token pattern, run on one sentence. Each
match prints its label, its token range, and its text, so the reader can see
which tokens the pattern covered.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::MatcherPattern city{.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false};
  nlp::MatcherPattern ship{.label = "ACTION", .tokens = {"ship"}};

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::matcher({city, ship})}};
  const nlp::Doc doc = wf.infer("We ship from New York and shipped to New York.");

  for (const nlp::Span& match : doc.matches) {
    std::cout << match.label << " tokens [" << match.token_start << ", " << match.token_end
              << ") text=\"" << match.text(doc) << "\"\n";
  }
  return 0;
}
```

Output

```text
ACTION tokens [1, 2) text="ship"
CITY tokens [3, 5) text="New York"
CITY tokens [8, 10) text="New York"
```

### Example 2

The `use_lemmas` flag on the same sentence. The pattern `{"write"}` matches
`wrote` and `writes` only when lemmas are used, because the lemma of each is
`write`. The pattern `{"wrote"}` with lemmas on never matches, because no
token's lemma is `wrote`. With lemmas off, the surface pattern matches its
exact spelling only.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "She wrote notes and he writes reports.";
  const std::pair<const char*, bool> cases[] = {
      {"write", true}, {"wrote", true}, {"wrote", false}, {"write", false},
  };
  for (const auto& [word, lemmas] : cases) {
    nlp::MatcherPattern rule{.label = "WRITE", .tokens = {word}, .use_lemmas = lemmas};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::matcher({rule})}};
    const nlp::Doc doc = wf.infer(text);

    std::cout << "pattern=\"" << word << "\" use_lemmas=" << (lemmas ? "true" : "false")
              << " matches=" << doc.matches.size() << "\n";
    for (const nlp::Span& match : doc.matches) {
      std::cout << "  " << match.text(doc) << "\n";
    }
  }
  return 0;
}
```

Output

```text
pattern="write" use_lemmas=true matches=2
  wrote
  writes
pattern="wrote" use_lemmas=true matches=0
pattern="wrote" use_lemmas=false matches=1
  wrote
pattern="write" use_lemmas=false matches=0
```

### Example 3

A pattern's token strings are compared as written, after normalization by the
stage's own `lowercase` option. A pattern `{"New", "York"}` finds `new york`
in lowercase text only when the stage lowercases both sides.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::MatcherPattern city{.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false};
  for (const bool lowercase : {true, false}) {
    nlp::MatcherOptions options{.lowercase = lowercase};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::matcher({city}, options)}};
    const nlp::Doc doc = wf.infer("i moved to new york.");

    std::cout << "lowercase = " << (lowercase ? "true" : "false") << " -> " << doc.matches.size()
              << " match(es)\n";
  }
  return 0;
}
```

Output

```text
lowercase = true -> 1 match(es)
lowercase = false -> 0 match(es)
```

## Changelog

- **0.1** -- initial documented surface.
