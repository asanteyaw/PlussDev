# `pluss::ai::nlp::Morphology`

A token's or lexeme's cached shape, casing, and character-class
analysis. It is computed once, when a surface form is first observed,
then shared by every occurrence of that form through
[`Lexicon::intern_morphology`](Lexicon/intern_morphology.md) -- so reading
`Token::morphology` is a copy of already-computed flags, not a fresh
analysis.

## Fields

### `casing`

`CaseKind`, default `None`. The token's overall casing pattern, as
described above. A numeric token with no letters is `None`, since casing
does not apply to it. Read it together with the boolean
`is_titlecase`/`is_uppercase`/`is_lowercase` flags, which give the same
information as independent checks.

### `kind`

`TokenKind`, default `Unknown`. The coarse category of the surface form.
Tokenization assigns it from the character classes present; `Punct` is
assigned only to tokens made entirely of punctuation.

### `length`

`std::int32_t`, default `0`. The surface form's length in characters.

### `is_ascii`

`bool`, default `true`. `false` when the surface form contains any
non-ASCII byte, which matters for languages with accented letters.

### `has_alpha`

`bool` flags, each default `false`. Whether the surface form contains at
least one letter, digit, hyphen, or apostrophe. They are independent
checks, not mutually exclusive: `has_alpha` and `has_digit` are both
true for `pluss2`.

### `has_digit`

`bool` flags, each default `false`. Whether the surface form contains at
least one letter, digit, hyphen, or apostrophe. They are independent
checks, not mutually exclusive: `has_alpha` and `has_digit` are both
true for `pluss2`.

### `has_hyphen`

`bool` flags, each default `false`. Whether the surface form contains at
least one letter, digit, hyphen, or apostrophe. They are independent
checks, not mutually exclusive: `has_alpha` and `has_digit` are both
true for `pluss2`.

### `has_apostrophe`

`bool` flags, each default `false`. Whether the surface form contains at
least one letter, digit, hyphen, or apostrophe. They are independent
checks, not mutually exclusive: `has_alpha` and `has_digit` are both
true for `pluss2`.

### `is_titlecase`

`bool` flags, each default `false`. Whole-word casing checks that match
`casing` but are cheaper to test in a branch. `is_uppercase` is true for
`NASA`; `is_titlecase` for `Laurel`; `is_lowercase` for `api`.

### `is_uppercase`

`bool` flags, each default `false`. Whole-word casing checks that match
`casing` but are cheaper to test in a branch. `is_uppercase` is true for
`NASA`; `is_titlecase` for `Laurel`; `is_lowercase` for `api`.

### `is_lowercase`

`bool` flags, each default `false`. Whole-word casing checks that match
`casing` but are cheaper to test in a branch. `is_uppercase` is true for
`NASA`; `is_titlecase` for `Laurel`; `is_lowercase` for `api`.

### `is_number_like`

`bool` flags, each default `false`. Category checks behind `kind`.
`is_number_like` is true for `42`. The tokenizer splits decimal and
grouped numbers into separate tokens (`3.5` becomes `3`, `.`, `5`;
`1,000` becomes `1`, `,`, `000`), so a single token is never a full
decimal. `is_url`/`is_email` are true for the matching shapes.
`is_punct` is true when the whole surface form is punctuation.

### `is_url`

`bool` flags, each default `false`. Category checks behind `kind`.
`is_number_like` is true for `42`. The tokenizer splits decimal and
grouped numbers into separate tokens (`3.5` becomes `3`, `.`, `5`;
`1,000` becomes `1`, `,`, `000`), so a single token is never a full
decimal. `is_url`/`is_email` are true for the matching shapes.
`is_punct` is true when the whole surface form is punctuation.

### `is_email`

`bool` flags, each default `false`. Category checks behind `kind`.
`is_number_like` is true for `42`. The tokenizer splits decimal and
grouped numbers into separate tokens (`3.5` becomes `3`, `.`, `5`;
`1,000` becomes `1`, `,`, `000`), so a single token is never a full
decimal. `is_url`/`is_email` are true for the matching shapes.
`is_punct` is true when the whole surface form is punctuation.

### `is_punct`

`bool` flags, each default `false`. Category checks behind `kind`.
`is_number_like` is true for `42`. The tokenizer splits decimal and
grouped numbers into separate tokens (`3.5` becomes `3`, `.`, `5`;
`1,000` becomes `1`, `,`, `000`), so a single token is never a full
decimal. `is_url`/`is_email` are true for the matching shapes.
`is_punct` is true when the whole surface form is punctuation.

### `norm`

`std::string`, default empty. The surface text lowercased. Punctuation
tokens keep their own character, so `norm` for `.` is `.`. For `Laurel`
it is `laurel`, and for `NASA` it is `nasa`.

### `shape`

`std::string`, default empty. A character-by-character shape signature:
every uppercase letter becomes `X`, every lowercase letter `x`, every
digit `d`, and any other character is kept as itself. Runs are not
collapsed, so `Laurel` becomes `Xxxxxx` and `pluss2` becomes `xxxxxd`.
Two words with the same shape share a capitalization and digit pattern
regardless of their letters, which makes the shape a useful feature for
classical models.

### `prefix`

`std::string`, default empty. The first and last three characters of
the surface form, as cached forms a feature extractor can read directly.
For `Laurel` they are `Lau` and `rel`.

### `suffix`

`std::string`, default empty. The first and last three characters of
the surface form, as cached forms a feature extractor can read directly.
For `Laurel` they are `Lau` and `rel`.

### `spec`

`std::string`, default empty until tokenization fills it. A single
human-readable summary of the flags above, pipe-separated, in a fixed
order: `kind`, `case`, `ascii`, `alpha`, `digit`, `hyphen`,
`apostrophe`. For `NASA` it reads
`kind=word|case=upper|ascii=1|alpha=1|digit=0|hyphen=0|apostrophe=0`.
It is intended for logging and diagnostics; read the typed fields for
logic.

## Guidance

Read `kind` and `casing` first; they summarize the rest. The boolean
flags are cheaper to test in a branch than comparing enums, and both
answer the same questions. `shape` and `prefix`/`suffix` are the features
to reach for when building a classical model over tokens.

The two enumerations this struct uses are:

- `CaseKind`: `None`, `Lower`, `Upper`, `Title`, `Mixed`. `Title` means
  the first letter is uppercase and the rest lowercase (`Laurel`);
  `Mixed` means a case pattern none of the others describe.
- `TokenKind`: `Unknown`, `Word`, `Number`, `Url`, `Email`, `Punct`,
  `Mixed`. The kind is the single most useful summary: a token can be
  alphabetic and digit-bearing at once (`pluss2`), which `Mixed` covers.

## See Also

- [`Lexicon::intern_morphology`](Lexicon/intern_morphology.md) -- how
  identical morphology is deduplicated across occurrences.
- [`Token`](Token.md) and [`Lexeme`](Lexeme.md) -- the two records that
  carry a `Morphology`.

## Practical Examples

These examples read morphology from a tokenized document, so the values
come from a real analysis rather than hand-built structs.

### Example 1

Reading the cached shape, prefix, suffix, and normalized form of a
titlecase word.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");

  const nlp::Lexeme* laurel = doc.lexicon->find("Laurel");
  std::cout << "shape=" << laurel->morphology.shape << "\n";
  std::cout << "prefix=" << laurel->morphology.prefix << " suffix=" << laurel->morphology.suffix << "\n";
  std::cout << "norm=" << laurel->morphology.norm << "\n";
  return 0;
}
```

Output

```text
shape=Xxxxxx
prefix=Lau suffix=rel
norm=laurel
```

### Example 2

Telling apart an uppercase acronym, an email address, and a number by
their kind and casing flags.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");

  const nlp::Morphology& nasa = doc.lexicon->find("NASA")->morphology;
  const nlp::Morphology& email = doc.lexicon->find("help@pluss.dev")->morphology;
  const nlp::Morphology& number = doc.lexicon->find("42")->morphology;

  std::cout << "nasa uppercase=" << nasa.is_uppercase << " kind=" << static_cast<int>(nasa.kind) << "\n";
  std::cout << "email is_email=" << email.is_email << "\n";
  std::cout << "number is_number_like=" << number.is_number_like << " has_digit=" << number.has_digit << "\n";
  return 0;
}
```

Output

```text
nasa uppercase=1 kind=1
email is_email=1
number is_number_like=1 has_digit=1
```

### Example 3

Reading the casing enum and the length of a word, to show that the enum
is a plain integral value a caller can branch on.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");

  const nlp::Morphology& laurel = doc.lexicon->find("Laurel")->morphology;
  std::cout << "casing=" << static_cast<int>(laurel.casing) << " length=" << laurel.length << "\n";
  return 0;
}
```

Output

```text
casing=3 length=6
```

## Changelog

- **0.1** -- initial documented surface.
