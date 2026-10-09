# `pluss::ai::nlp::Token`

One token occurrence's full annotation inside a [`Doc`](Doc.md): where
it sits in the text, its interned string ids, its surface form and
analysis strings, its punctuation and sentence-boundary flags, and its
[`Morphology`](Morphology.md). Every stage after tokenization reads or
fills some of these fields in place.

Tokens are produced by [`RuleTokenizerStage`](../tasks/RuleTokenizerStage.md)
and then annotated by the rest of the pipeline; a caller reads them from
`Doc::tokens` rather than constructing them.

## Fields

### `index`

`std::int64_t`, default `0`. The token's position in `Doc::tokens`, from
`0`. A token's `index` always equals its position in the vector, which
is what makes [`children_of`](Doc/children_of.md) and the other
dependency methods able to address tokens by number.

### `char_start`

`std::int64_t`, default `0`. The offset of the token's first character
in `Doc::text`. Together with `char_end` it locates the token exactly, so
[`Span::text`](Span.md) can slice a multi-token span straight out of the
original string.


### `char_end`

`std::int64_t`, default `0`. The offset one past the token's last character,
so `char_end - char_start` is the token's length in the text. Whitespace
between two tokens is the gap from one token's `char_end` to the next
token's `char_start`.


### `chief`

`std::int64_t`, default `-1` on a bare `Token`. The index of this
token's dependency head. Tokenization sets it to the token's own index,
making every token its own head until [`SyntaxStage`](../tasks/SyntaxStage.md)
assigns real heads. The dependency methods on [`Doc`](Doc.md) read this
field, so changing it by hand changes what they report.

### `orth_id`

`StringId`, default `0`. The interned id of the surface text. Every
occurrence of the same surface form carries the same `orth_id`, because
all of them are interned in one [`TextPool`](TextPool.md). Compare ids when
checking many tokens for equality, and use `orth` only when the text is
needed.


### `lower_id`

`StringId`, default `0`. The interned id of the lowercase form. Tokens
that differ only in case (`Laurel` and `laurel`) have different `orth_id`s
but the same `lower_id`, which makes case-insensitive grouping an integer
comparison.


### `norm_id`

`StringId`, default `0`. The interned id of the normalized form, the
lowercased text with punctuation kept as its own character. Use it when
a feature should ignore capitalization.


### `prefix_id`

`StringId`, default `0`. The interned id of the first three characters of
the surface form. Tokens sharing a prefix share this id, so prefix
features can be grouped without comparing strings.


### `suffix_id`

`StringId`, default `0`. The interned id of the last three characters of
the surface form, the suffix counterpart to `prefix_id`.


### `lemma_id`

`StringId`, default `0`. The interned id of the lemma string. Set by
[`LemmatizerStage`](../tasks/LemmatizerStage.md); zero when no lemmatizer
has run for this token.


### `morphology_id`

`StringId`, default `0`. The id of the token's `Morphology` record in the
lexicon's [`MorphBank`](MorphBank.md). Set during tokenization. A token's
`morphology_id` repeats for every occurrence of the same surface form.


### `orth`

`std::string`, default empty. The token's surface text, exactly as it
appears in the document. This is the string [`Doc::token_texts`](Doc/token_texts.md)
returns.

### `lemma`

`std::string`, default empty. The token's lemma, filled in by
[`LemmatizerStage`](../tasks/LemmatizerStage.md). Empty when no
lemmatizer ran, which is why [`Doc::lemma_texts`](Doc/lemma_texts.md)
returns empty strings in that case.

### `pos`

`std::string`, default empty. The part-of-speech tag, such as `PROPN`,
`VERB`, `NOUN`, or `PUNCT`. Filled in by [`TaggerStage`](../tasks/TaggerStage.md)
only; a pipeline without a tagger leaves it empty.

### `role`

`std::string`, default empty. The dependency label of this token's
relation to its head, such as `root`, `nsubj`, `obj`, `prep`, or
`pobj`. Filled in by [`SyntaxStage`](../tasks/SyntaxStage.md) only, and
always empty without a syntax stage.

### `entity_label`

`std::string`, default empty. The entity type this token belongs to,
such as `PERSON` or `ORG`, filled in by [`NerStage`](../tasks/NerStage.md)
when a token falls inside a recognized entity. Empty for tokens outside
every entity. [`Doc::entities`](Doc.md) gives the same information as
spans; this field gives it per token.

### `is_punct`

`bool`, default `false`. `true` for tokens made entirely of punctuation,
such as `.` or `!`. Set during tokenization, so it is available in every
pipeline.

### `sentence_start`

`bool`, default `false`. `true` on the first token of a sentence, as set by
[`SentencizerStage`](../tasks/SentencizerStage.md). Stays `false` in every
pipeline without a sentencizer.


### `sentence_end`

`bool`, default `false`. `true` on the final token of a sentence, typically
the terminating punctuation.


### `space_after`

`bool`, default `false`. `true` when whitespace follows this token in
the original text. Set during tokenization, so `APIs.` gives `APIs` a
`space_after` of `false` and `.` a `space_after` of `true` when a space
follows the period. A detokenizer uses this flag to rebuild the original
spacing.

### `morphology`

`Morphology`, default-constructed. This token's own copy of its
morphology analysis, so flags and shape strings can be read directly
from the token. `morphology_id` identifies the same record in the
lexicon's [`MorphBank`](MorphBank.md); prefer it when comparing many
tokens for shared analysis.

## Guidance

Read `orth`, `index`, and the character offsets for alignment and
display; read `lemma`, `pos`, and the sentence flags for linguistic
features; read `role` and `chief` only after a syntax stage. A caller
who needs spans rather than per-token flags should use
[`Doc::sentences`](Doc.md), [`Doc::entities`](Doc.md), or
[`Doc::branch_of`](Doc/branch_of.md) instead of scanning tokens.

## See Also

- [`Doc`](Doc.md) -- owns the `tokens` vector.
- [`Morphology`](Morphology.md) -- the `morphology` field's type.
- [`Lexeme`](Lexeme.md) -- the type-level record for the same surface form.

## Practical Examples

### Example 1

Reading the per-token fields a tagged, sentence-split pipeline fills in.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize(), nlp::stage::tagger()},
  };
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs. Second sentence!");

  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " pos=" << token.pos << " punct=" << token.is_punct
              << " sentence_start=" << token.sentence_start << "\n";
  }
  return 0;
}
```

Output

```text
Dr. pos=PROPN punct=0 sentence_start=1
Laurel pos=PROPN punct=0 sentence_start=0
writes pos=VERB punct=0 sentence_start=0
APIs pos=NOUN punct=0 sentence_start=0
. pos=PUNCT punct=1 sentence_start=0
Second pos=PROPN punct=0 sentence_start=1
sentence pos=NOUN punct=0 sentence_start=0
! pos=PUNCT punct=1 sentence_start=0
```

### Example 2

Reconstructing the original spacing from `space_after`, which the
tokenizer sets for every token.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  std::string rebuilt;
  for (const nlp::Token& token : doc.tokens) {
    rebuilt += token.orth;
    if (token.space_after) rebuilt += " ";
  }
  std::cout << "rebuilt=" << rebuilt << "\n";
  return 0;
}
```

Output

```text
rebuilt=Laurel writes APIs.
```

### Example 3

The character offsets slice each token's exact surface text back out of
the document.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  const nlp::Token& writes = doc.tokens[1];
  std::cout << "chars=" << writes.char_start << "-" << writes.char_end << "\n";
  std::cout << "slice=" << doc.text.substr(static_cast<std::size_t>(writes.char_start),
                                          static_cast<std::size_t>(writes.char_end - writes.char_start)) << "\n";
  return 0;
}
```

Output

```text
chars=7-13
slice=writes
```

## Changelog

- **0.1** -- initial documented surface.
