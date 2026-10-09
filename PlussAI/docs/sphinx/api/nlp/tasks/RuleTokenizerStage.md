# `pluss::ai::nlp::RuleTokenizerStage`

`RuleTokenizerStage` turns `Doc::text` into the token stream that every later
lexical stage reads. It is the only stage whose input is raw text, so it is
the first stage in almost every workflow. For each token it records the
half-open character span, the surface text, the interned lexical ids, the
morphology analysis, and whether the token is punctuation. It also records
whether a space follows the token, so the original text can be reconstructed
from the token stream.

The segmentation is rule-based. The stage has no training step and keeps no
learned state. Its behavior is fully determined by its two options and the
language of the document.

Running the stage again is a reset, not an append. Every token, sentence,
phrase, match, entity, document label, and document score is cleared, and the
document's lexicon is emptied before tokenization runs again. Stage order
matters for the same reason: a later stage that depended on an earlier
annotation must be run after the tokenizer has rebuilt the stream.

## Signature

```cpp
pluss::ai::nlp::RuleTokenizerStage::RuleTokenizerStage(
    pluss::ai::nlp::RuleTokenizerOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::tokenize(
    pluss::ai::nlp::RuleTokenizerOptions options = {});
```

## Construction

Use `stage::tokenize` when the stage only needs to be placed in a
`Workflow`. It returns a `Stage`, which is a shared pointer to the stage.

Construct the concrete class directly when you want to keep a typed handle to
it, or when you want to hand it to code that works with `RuleTokenizerStage`
rather than `IStage`. The two forms behave identically:

```cpp
auto tokenizer = std::make_shared<pluss::ai::nlp::RuleTokenizerStage>(
    pluss::ai::nlp::RuleTokenizerOptions{.keep_punct = false});
pluss::ai::nlp::Workflow wf{.stages = {tokenizer}};
```

The constructor does no work beyond storing the options. Nothing is validated
at construction because no option has an invalid value.

## Options

The options struct is `pluss::ai::nlp::RuleTokenizerOptions`. It has two
boolean fields, and both default to `true`.

### `keep_punct`

Controls whether punctuation characters become tokens of their own. With
`true`, a comma, exclamation point, or period after a word is its own token,
and its `Token::is_punct` is `true`. With `false`, those characters are dropped
from the token stream. The characters are still present in `Doc::text`, and
each surviving token keeps its original character offsets, so the gap between
two neighboring tokens shows where a dropped mark used to be.

Turn this off for bag-of-words style features, where punctuation only adds
noise. Keep it on when a later stage needs sentence boundaries, when a
sentencizer must see the marks that end a sentence, or when the token stream
must cover every visible character.

Punctuation inside a word is never split off by this option. The apostrophe
in `Laurel's`, the hyphen in `code-review`, the dots inside a URL, and the at
sign inside an email address stay part of their token, whatever this option
says. Only punctuation that stands alone as its own unit is affected.

### `use_language_exceptions`

Controls whether the stage merges two adjacent tokens when the document's
language pack says the pair is one unit. Each language pack carries a short,
case-sensitive list of such pairs. The English list merges `Dr`, `Mr`, `Mrs`,
`Ms`, and `Prof` followed by `.` into `Dr.`, `Mr.`, `Mrs.`, `Ms.`, and `Prof.`.
The French, Spanish, and German lists cover their own honorifics and
abbreviations.

The merge only fires when the splitter has already produced the two pieces as
separate tokens. An abbreviation written without a space, such as `Dr. Ada`, is already kept whole by the splitter, so this option has no
visible effect on it. The option matters when the input spaces the period
out, as in `Dr . Ada`, or when text was produced by a process that separates
every mark. A merged token keeps the character span of both pieces, and its
morphology and lexical ids are recomputed from the merged surface.

Leave this on for ordinary text. Turn it off when you need the raw split
exactly as the splitter produced it, for example when auditing how a corpus
was segmented.

## State

The stage keeps only its options. It does not override `learn`, so calling
`Workflow::learn` on a pipeline that contains only this stage does nothing
to it.

`dump_state_lines()` returns exactly two lines, one per option, in the form
`keep_punct=1` or `keep_punct=0`, followed by `use_language_exceptions=1` or
`use_language_exceptions=0`. `load_state_lines()` accepts only those two
keys. Any other line is rejected with an error that names the offending
line.

## Methods

### `name()`

Returns `"rule_tokenizer"`, the stable registry name. Use it as the key when
looking up the stage in a registry or when reading a saved workflow, not as a
display label.

### `apply(Doc&)`

Tokenizes `Doc::text` in place. It clears the token stream and every
downstream annotation first, then splits the text into spans, interns each
surface form into the document's lexicon, computes the morphology of each
token, applies language merges when enabled, and records character offsets
and the trailing-space flag.

Empty text produces an empty token stream, and the other annotations are still
cleared.

### `dump_state_lines()`

Returns the two option lines described above. The result is stable across
runs and can be stored next to a saved workflow.

### `load_state_lines(lines)`

Replaces the two options from a list of lines. Lines not recognized by the
stage are rejected, so a saved file from a different stage cannot be applied
to this one by accident. Calling it with the output of `dump_state_lines()`
restores the same configuration.

## Practical Examples

These examples build a tokenizer, apply it through a workflow, and print what
it produced. The first shows the default token stream on text with contractions,
hyphens, a URL, and an email address. The second shows what the punctuation
option changes. The third shows the language exceptions for spaced honorifics
in two languages. The fourth saves the configuration and restores it into a
new stage.

### Example 1

The default token stream keeps contractions, hyphenated words, a URL, and an
email address whole, and keeps each punctuation mark as its own token. Each
token prints its index, its surface, and its character span.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel's code-review posted https://pluss.dev/docs, then help@pluss.dev replied.");

  std::cout << "tokens = " << doc.tokens.size() << "\n\n";
  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.index << ": " << token.orth << "  [" << token.char_start << ", " << token.char_end << ")";
    if (token.is_punct) std::cout << "  punct";
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
tokens = 9

0: Laurel's  [0, 8)
1: code-review  [9, 20)
2: posted  [21, 27)
3: https://pluss.dev/docs  [28, 50)
4: ,  [50, 51)  punct
5: then  [52, 56)
6: help@pluss.dev  [57, 71)
7: replied  [72, 79)
8: .  [79, 80)  punct
```

### Example 2

The same sentence through two tokenizers that differ only in the punctuation
option. With the option off, the commas, the colon, and the exclamation point disappear from the
token stream, but the offsets of the surviving tokens still show where each
dropped mark sat in the original text.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string_view>

int main() {
  namespace nlp = pluss::ai::nlp;

  const std::string_view text = "Ship it now, then tell Kofi: done!";
  for (const bool keep : {true, false}) {
    nlp::RuleTokenizerOptions options{.keep_punct = keep};
    nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(options)}};
    const nlp::Doc doc = wf.infer(text);

    std::cout << "keep_punct = " << (keep ? "true" : "false") << ", tokens = " << doc.tokens.size() << "\n";
    for (const nlp::Token& token : doc.tokens) {
      std::cout << "  " << token.orth << "  [" << token.char_start << ", " << token.char_end << ")\n";
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
keep_punct = true, tokens = 10
  Ship  [0, 4)
  it  [5, 7)
  now  [8, 11)
  ,  [11, 12)
  then  [13, 17)
  tell  [18, 22)
  Kofi  [23, 27)
  :  [27, 28)
  done  [29, 33)
  !  [33, 34)

keep_punct = false, tokens = 7
  Ship  [0, 4)
  it  [5, 7)
  now  [8, 11)
  then  [13, 17)
  tell  [18, 22)
  Kofi  [23, 27)
  done  [29, 33)
```

### Example 3

The language exceptions merge a spaced honorific with its period, and the
merge is language-specific. The French pack merges `M` and `.`, so the
spaced input becomes `M.` only when the document is French. The same
option is then disabled to show the raw split.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool exceptions : {true, false}) {
    nlp::RuleTokenizerOptions options{.use_language_exceptions = exceptions};
    nlp::Workflow wf{.lang = nlp::Language::French, .stages = {nlp::stage::tokenize(options)}};
    const nlp::Doc doc = wf.infer("M . Dupont est là.");

    std::cout << "use_language_exceptions = " << (exceptions ? "true" : "false") << "\n";
    for (const std::string& text : doc.token_texts()) {
      std::cout << "  [" << text << "]\n";
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
use_language_exceptions = true
  [M.]
  [Dupont]
  [est]
  [là]
  [.]

use_language_exceptions = false
  [M]
  [.]
  [Dupont]
  [est]
  [là]
  [.]
```

### Example 4

A configuration saved with `dump_state_lines()` restores into a fresh stage.
Restoring the lines gives the same output for the same input, which is how a
saved workflow reproduces its tokenization.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::RuleTokenizerStage original{nlp::RuleTokenizerOptions{.keep_punct = false}};
  const std::vector<std::string> saved = original.dump_state_lines();
  std::cout << "saved lines:\n";
  for (const std::string& line : saved) std::cout << "  " << line << "\n";

  const std::vector<std::string_view> views(saved.begin(), saved.end());
  auto restored = std::make_shared<nlp::RuleTokenizerStage>();
  restored->load_state_lines(views);

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {restored}};
  const nlp::Doc doc = wf.infer("Ship it now, then tell Kofi.");
  std::cout << "restored tokens = " << doc.tokens.size() << "\n";
  std::cout << "restored name = " << restored->name() << "\n";
  return 0;
}
```

Output

```text
saved lines:
  keep_punct=0
  use_language_exceptions=1
restored tokens = 6
restored name = rule_tokenizer
```

## See Also

- `stage::tokenize` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`SentencizerStage`](SentencizerStage.md) -- the next stage, which relies on the punctuation tokens this stage emits.
- [`LemmatizerStage`](LemmatizerStage.md) -- fills lemmas for the tokens this stage creates.
- [`Doc`](../core/Doc.md) -- the document the stage rewrites.
- [`Token`](../core/Token.md) -- the per-token record this stage fills.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
