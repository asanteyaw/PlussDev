# `pluss::ai::nlp::LemmatizerStage`

`LemmatizerStage` fills the `lemma` and `lemma_id` fields of every token. A
lemma is the dictionary-style base form of a word: `wrote` and `written` share
the lemma `write`, and `books` shares the lemma `book`. Stages that compare
word forms, such as the matcher, the tagger, the NER pattern rules, and the
text classifier, read the lemma when they are told to use lemmas. They fall
back to the surface form when a token's lemma is empty, so the lemmatizer is
optional. Without it, those stages compare surface forms only.

Each token's lemma comes from one of two sources, tried in order. The first
is a language table of irregular forms, which covers the forms a rule cannot
guess: `wrote`, `were`, `sommes`, `fue`, `war`. The second is a small
suffix-based fallback that handles regular plurals, past tenses, and
progressive forms. Punctuation tokens are handled separately, and their lemma
is controlled by one option described below.

The fallback is a heuristic and is not a morphological analyzer. It removes a
suffix when the removal is cheap and predictable, so it is right for
`books`, `studies`, and `stopped`, and wrong for `writing`, which becomes
`writ`. The language table is what fixes the common irregular cases, and the
Practical Examples show where the two sources disagree.

## Signature

```cpp
pluss::ai::nlp::LemmatizerStage::LemmatizerStage(
    pluss::ai::nlp::LemmatizerOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::lemmatize(
    pluss::ai::nlp::LemmatizerOptions options = {});
```

## Construction

Use `stage::lemmatize` in a workflow. The constructor stores the options and
does no validation.

The lemmatizer needs tokens, so place it after a tokenizer. It does not need
a sentencizer. If it runs before a tagger, the tagger's lemma-based rules see
the lemmas; if it runs after one, the tagger sees empty lemmas and falls back
to surface forms.

## Options

The options struct is `pluss::ai::nlp::LemmatizerOptions`. All three fields
default to `true`.

### `lowercase`

Controls the case of the rule-based fallback. When `true`, the fallback
lowercases the token before it applies its rules, so `Books` becomes `book`
and `WROTE` would become `wrote` if the table did not already cover it.

When `false`, the fallback returns the surface form unchanged. No suffix is
removed, so `Books` stays `Books`. The language table is not
affected by this option. A table lemma is already canonical and is applied
whether `lowercase` is on or off.

Turn this off when you want the fallback to leave words alone and rely only
on the table. Turn it on, which is the default, for most text-level features,
since the tagger and the matcher compare lowercase forms.

### `fill_from_orth_when_empty`

Controls what happens when no lemma is found. For a word token, the stage
always produces a lemma, because the fallback returns a non-empty string for
any non-empty word. The option matters for punctuation tokens, which the
stage does not lemmatize.

When `true`, a punctuation token's lemma is its own surface text, so a period
has the lemma `.` and a lemma id that names it. When `false`, a punctuation
token's lemma stays empty and its `lemma_id` is `0`. The option also covers
the unusual case of a word whose fallback comes back empty, which then takes
its surface form.

Turn it off when you need the lemma field to say "no lemma" for punctuation,
for example in a count of distinct lemmas, where a column of periods would
otherwise inflate the total.

### `use_language_lookup`

Controls whether the language table is consulted before the fallback. When
`true`, the stage looks up the token's surface text in the table for the
document's language, using its lowercase form as the key. A hit sets the
lemma and skips the fallback entirely.

When `false`, the table is ignored and every word goes through the fallback.
That changes results for irregular forms: `wrote` falls through the
fallback's rules and stays `wrote`, because no regular suffix applies, and the
French `sommes` becomes `somme` instead of `etre`. Turn the option off to
measure what the table adds, or when a corpus is so unlike the table's source
that its entries would be wrong.

## State

The stage keeps only its three options. It does not override `learn`.

`dump_state_lines()` returns `lowercase`, `fill_from_orth_when_empty`, and
`use_language_lookup`, each set to `1` or `0`. `load_state_lines()` accepts
those six lines in either value and rejects any other line with an error that
names it.

## Methods

### `name()`

Returns `"lemmatizer"`.

### `apply(Doc&)`

Walks the tokens in order. Each punctuation token gets its lemma from the
`fill_from_orth_when_empty` rule. Each other token gets the language-table
lemma when that lookup is enabled and hits; otherwise it gets the fallback
result, which is the surface form when `lowercase` is off. A lemma that is
still empty is filled from the surface form when `fill_from_orth_when_empty`
is on. Every non-empty lemma is interned into the document's lexicon, and the
token's `lemma_id` is set to that id. An empty lemma sets `lemma_id` to `0`.

`apply` does not clear the tokens' other fields. It overwrites only the lemma
and lemma id of each token, so it can be run again after the options change.

### `dump_state_lines()`

Returns the three option lines described under State.

### `load_state_lines(lines)`

Replaces the three options from a list of lines. A line it does not recognize
is an error.

## Practical Examples

These examples show the default lemmas on a mixed sentence, the case toggle,
the language table against the fallback in French, and the punctuation rule.
Each one prints the surface form next to its lemma, so the reader can compare
them directly.

### Example 1

The default lemmatizer on a sentence with irregular forms, regular plurals,
a past tense, a progressive form, and a capitalized proper noun. The output
shows where the table supplies the lemma and where the fallback does. Note
that `writing` becomes `writ`, which is the fallback's known limitation.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel wrote books, studies were stopped, and writing continues.");

  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " -> " << token.lemma << "\n";
  }
  return 0;
}
```

Output

```text
Laurel -> laurel
wrote -> write
books -> book
, -> ,
studies -> study
were -> be
stopped -> stop
, -> ,
and -> and
writing -> writ
continues -> continue
. -> .
```

### Example 2

The `lowercase` toggle on capitalized and uppercase input. With the option on,
the fallback lowercases its result. With it off, the fallback returns the
surface form. The language table still applies in both runs, so `WROTE`
becomes `write` either way.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool lowercase : {true, false}) {
    nlp::LemmatizerOptions options{.lowercase = lowercase};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(options)}};
    const nlp::Doc doc = wf.infer("Laurel WROTE Books.");

    std::cout << "lowercase = " << (lowercase ? "true" : "false") << "\n";
    for (const nlp::Token& token : doc.tokens) {
      std::cout << "  " << token.orth << " -> " << token.lemma << "\n";
    }
  }
  return 0;
}
```

Output

```text
lowercase = true
  Laurel -> laurel
  WROTE -> write
  Books -> book
  . -> .
lowercase = false
  Laurel -> Laurel
  WROTE -> write
  Books -> Books
  . -> .
```

### Example 3

The language table against the fallback. In French, the table maps `sommes`
to `etre`. With `use_language_lookup` off, the fallback removes the final
`s` and returns `somme`, which is a different word. The table result is the
correct lemma for this use. The fallback also gets `Nous` wrong, reducing it to
`nou`, which is a second reason to rely on the table for a language whose
common forms it covers.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool lookup : {true, false}) {
    nlp::LemmatizerOptions options{.use_language_lookup = lookup};
    nlp::Workflow wf{.lang = nlp::Language::French,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(options)}};
    const nlp::Doc doc = wf.infer("Nous sommes ici.");

    std::cout << "use_language_lookup = " << (lookup ? "true" : "false") << "\n";
    for (const nlp::Token& token : doc.tokens) {
      std::cout << "  " << token.orth << " -> " << token.lemma << "\n";
    }
  }
  return 0;
}
```

Output

```text
use_language_lookup = true
  Nous -> nou
  sommes -> etre
  ici -> ici
  . -> .
use_language_lookup = false
  Nous -> nou
  sommes -> somme
  ici -> ici
  . -> .
```

### Example 4

The punctuation rule. With `fill_from_orth_when_empty` on, a punctuation token
gets its own text as its lemma, and that lemma has a non-zero lemma id. With
the option off, the lemma is empty and the lemma id is `0`, so a count of
distinct lemma ids does not include punctuation.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool fill : {true, false}) {
    nlp::LemmatizerOptions options{.fill_from_orth_when_empty = fill};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(options)}};
    const nlp::Doc doc = wf.infer("Ada wrote, then left.");

    std::cout << "fill_from_orth_when_empty = " << (fill ? "true" : "false") << "\n";
    for (const nlp::Token& token : doc.tokens) {
      if (!token.is_punct) continue;
      std::cout << "  \"" << token.orth << "\" lemma=\"" << token.lemma
                << "\" lemma_id_set=" << (token.lemma_id != 0) << "\n";
    }
  }
  return 0;
}
```

Output

```text
fill_from_orth_when_empty = true
  "," lemma="," lemma_id_set=1
  "." lemma="." lemma_id_set=1
fill_from_orth_when_empty = false
  "," lemma="" lemma_id_set=0
  "." lemma="" lemma_id_set=0
```

## See Also

- `stage::lemmatize` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`RuleTokenizerStage`](RuleTokenizerStage.md) -- produces the tokens this stage lemmatizes.
- [`MatcherStage`](MatcherStage.md) -- compares lemmas when a pattern asks for them.
- [`Token`](../core/Token.md) -- the `lemma` and `lemma_id` fields this stage fills.
- [`Lexeme`](../core/Lexeme.md) -- the lexicon record that holds a lemma's text.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
