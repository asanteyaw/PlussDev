# `pluss::ai::nlp::Lexeme`

The type-level record a [`Lexicon`](Lexicon.md) keeps for one distinct
surface form: its interned string ids, its surface, lowercase, lemma,
normalized, prefix, and suffix strings, and its
[`Morphology`](Morphology.md). Where a [`Token`](Token.md) is one
occurrence in one document, a `Lexeme` is the form itself, counted once
no matter how many times it appears.

A `Lexeme` is created the first time a `Lexicon` observes its surface
form (through [`observe`](Lexicon/observe.md) or tokenization) and is
then returned by reference on every later lookup. Callers read lexemes;
they do not construct them.

## Fields

### `id`

`StringId`, default `0`. The interned id of the surface text, the same
value a [`Token::orth_id`](Token.md) carries for every occurrence of that
form. Use it as the key for [`Lexicon::find`](Lexicon/find.md) when a
token's id is already at hand.

### `lower_id`

`StringId`, default `0`. The interned id of the lowercase form. Two
forms that differ only in case (`Laurel` and `laurel`) share a
`lower_id` but have different `id`s.

### `norm_id`

`StringId`, default `0`. The interned id of the normalized form in
[`norm`](#norm). Like `lower_id`, it groups forms that a normalizer
treats as equivalent.

### `prefix_id`

`StringId`, default `0`. The interned id of the lexeme's prefix string, so
lexemes that share an affix can be grouped by integer comparison.


### `suffix_id`

`StringId`, default `0`. The interned id of the lexeme's suffix string, the
counterpart to `prefix_id`.


### `morphology_id`

`StringId`, default `0`. The id of this lexeme's `Morphology` record in
the lexicon's [`MorphBank`](MorphBank.md). Passing it to
[`Lexicon::find_morphology`](Lexicon/find_morphology.md) returns the same
record as [`morphology`](#morphology) below.

### `text`

`std::string`, default empty. The surface form, exactly as first
observed. A lexeme is keyed by this text, so `Laurel` and `laurel` are
two different lexemes.

### `lower`

`std::string`, default empty. The surface form lowercased. For `Laurel`
it is `laurel`.

### `lemma`

`std::string`, default empty. The lemma, set by
[`LemmatizerStage`](../tasks/LemmatizerStage.md) when that stage observes
the form. Empty for a lexeme no lemmatizer has seen.

### `norm`

`std::string`, default empty. The lowercased form, with punctuation
tokens keeping their own character, matching the same field on
[`Morphology`](Morphology.md).

### `prefix`

`std::string`, default empty. The first three characters of the surface
form. For `Laurel` it is `Lau`.


### `suffix`

`std::string`, default empty. The last three characters of the surface form.
For `Laurel` it is `rel`.


### `morphology`

`Morphology`, default-constructed. This lexeme's morphology analysis: its
casing, kind, character-class flags, and shape. A copy of the record
[`morphology_id`](#morphology_id) identifies in the bank.

## Guidance

Read a lexeme when a question is about the word type: its shape or casing
pattern across a whole corpus, or its lemma. Read a `Token` when a
question is about one occurrence: its position, its part of speech in
context, or its sentence role. A feature that depends only on the surface
form (shape, prefix, suffix, lowercase identity) belongs on the lexeme,
computed once, and shared by every occurrence.

## See Also

- [`Lexicon`](Lexicon.md) -- stores and returns lexemes.
- [`Token`](Token.md) -- the per-occurrence counterpart.
- [`Morphology`](Morphology.md) -- the analysis a lexeme's `morphology`
  field holds.

## Practical Examples

### Example 1

Looking up a lexeme by its surface form and reading its lowercase, lemma,
and affix strings.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");

  const nlp::Lexeme* laurel = doc.lexicon->find("Laurel");
  std::cout << "text=" << laurel->text << " lower=" << laurel->lower << "\n";
  std::cout << "lemma=" << laurel->lemma << "\n";
  std::cout << "prefix=" << laurel->prefix << " suffix=" << laurel->suffix << "\n";
  return 0;
}
```

Output

```text
text=Laurel lower=laurel
lemma=laurel
prefix=Lau suffix=rel
```

### Example 2

A repeated surface form is one lexeme, so its `id` matches the id every
occurrence carries.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");

  const nlp::Lexeme* laurel = doc.lexicon->find("Laurel");
  std::int64_t occurrences = 0;
  for (const nlp::Token& token : doc.tokens) {
    if (token.orth_id == laurel->id) ++occurrences;
  }
  std::cout << "occurrences=" << occurrences << "\n";
  std::cout << "lexicon_size=" << doc.lexicon->size() << "\n";
  return 0;
}
```

Output

```text
occurrences=2
lexicon_size=12
```

### Example 3

Two forms that differ only in case are separate lexemes that share a
lowercase id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& upper = lex.observe("Hello");
  const nlp::Lexeme& lower = lex.observe("hello");
  std::cout << "same_lower_id=" << (upper.lower_id == lower.lower_id) << "\n";
  std::cout << "same_id=" << (upper.id == lower.id) << "\n";
  return 0;
}
```

Output

```text
same_lower_id=1
same_id=0
```

## Changelog

- **0.1** -- initial documented surface.
