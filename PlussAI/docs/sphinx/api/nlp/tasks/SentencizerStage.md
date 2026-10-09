# `pluss::ai::nlp::SentencizerStage`

`SentencizerStage` groups an already tokenized document into sentences. It
does not change the tokens' text or offsets. It writes one `Span` per sentence
into `Doc::sentences` with the label `"sentence"`, and it sets the
`sentence_start` and `sentence_end` flags on the first and last token of
each sentence. Later stages read those spans and flags, and the
[`Doc::sentence_texts()`](../core/Doc/sentence_texts.md) accessor returns the
text of each sentence.

Every sentence boundary comes from one of two rules: a token whose text ends
in a sentence-final mark, or a line break in the original text. Both rules are
configurable, and the stage rebuilds its sentence list from scratch on each
call, so running it twice does not double the sentences.

The sentencizer runs on tokens, so it must come after a tokenizer. It needs the
punctuation tokens that the tokenizer emits, which is the most important
interaction in this stage: see [`keep_punct`](RuleTokenizerStage.md) before
choosing a punctuation setting for the tokenizer.

## Signature

```cpp
pluss::ai::nlp::SentencizerStage::SentencizerStage(
    pluss::ai::nlp::SentencizerOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::sentencize(
    pluss::ai::nlp::SentencizerOptions options = {});
```

## Construction

Use `stage::sentencize` in a workflow. Construct the concrete class directly
when you need a typed handle, for example to read its options back through
`dump_state_lines()`.

The constructor stores the options and does no validation. Every combination
of the three boolean options is valid.

## Options

The options struct is `pluss::ai::nlp::SentencizerOptions`. All three fields
default to `true`, which gives the most common behavior: break on sentence-final
marks and on line breaks, and do not break after known abbreviations.

### `break_on_punct`

When `true`, a token whose text ends in `.`, `!`, or `?` closes the current
sentence after that token. This is the main rule. A sentence ends at `notes.`
and at `done!`, and the next token starts a new sentence.

When `false`, sentence-final marks have no effect. Only the line-break rule can
close a sentence, so a paragraph without line breaks becomes one sentence no
matter how many periods it contains. Turn this off when the input has
reliable line structure, such as one sentence per line, and the periods inside
it are not sentence boundaries.

The rule checks the token text, not the original characters. That is why
the tokenizer's punctuation setting matters: with
[`keep_punct`](RuleTokenizerStage.md) off, the period is no longer a token of
its own, and nothing ends the sentence. The Practical Examples show this case.

### `break_on_newline`

When `true`, a sentence closes at a line break between two neighboring tokens.
A break counts when the original text contains a line feed or a carriage
return between the end of one token and the start of the next. Spaces and tabs
do not count. The break works whether or not the words on either side end in
punctuation, so a headline or a list item with no final mark still ends its
own sentence.

When `false`, line breaks are ignored, and a sentence can run across several
lines of input. Use `false` for text where a line break is only a visual wrap
that happened to fall inside a sentence.

### `use_language_abbreviations`

When `true`, a token that ends in `.` and is listed as an abbreviation for the
document's language does not close a sentence. The lookup is case-insensitive.
The English list covers `dr.`, `mr.`, `mrs.`, `ms.`, `prof.`, `st.`, `vs.`,
`etc.`, `e.g.`, and `i.e.`. The French, Spanish, and German lists cover their
own titles and abbreviations.

Only the period is checked. A token that ends in `!` or `?` always closes a
sentence, whatever its text. When this option is `false`, every period closes
a sentence, including the period in `Dr.`, which is usually wrong for prose.

## State

The stage keeps only its three options. It does not override `learn`.

`dump_state_lines()` returns three lines in this order: `break_on_punct`,
`break_on_newline`, and `use_language_abbreviations`, each with the value `1`
or `0`. `load_state_lines()` accepts exactly those six lines, and any other
line is rejected with an error that names it.

## Methods

### `name()`

Returns `"sentencizer"`.

### `apply(Doc&)`

Clears `Doc::sentences` and both sentence flags on every token, then rebuilds
them. An empty token stream produces no sentences and leaves the flags cleared.

When `break_on_punct` is `false`, the whole token stream is one candidate
sentence, and `break_on_newline` can still split it. Otherwise the stage walks
the tokens in order and closes a sentence after each token that ends a sentence
by either rule. The last sentence always closes at the end of the stream, so
no token is left outside a sentence.

### `dump_state_lines()`

Returns the three option lines described under State.

### `load_state_lines(lines)`

Replaces the three options from a list of lines. It does not clear sentence
data already on a document; the next `apply` call rebuilds that.

## Practical Examples

These examples show the default behavior on a short paragraph, the effect of
each boundary rule, the abbreviation rule, and the punctuation interaction.
Each one prints sentence texts through `Span::text`, so the boundaries are
visible in the output.

### Example 1

The default sentencizer on a paragraph that contains an abbreviation and a
line break. The abbreviation `Dr.` does not end a sentence, and the period after
`notes` does. The line break after `Kofi` closes it as a sentence of one word,
and `agreed!` closes at its exclamation mark.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const nlp::Doc doc = wf.infer("Dr. Ada wrote notes. Kofi\nagreed!");

  std::cout << "sentences = " << doc.sentences.size() << "\n";
  for (const nlp::Span& sentence : doc.sentences) {
    std::cout << "  tokens [" << sentence.token_start << ", " << sentence.token_end << ") "
              << "label=" << sentence.label << " text=\"" << sentence.text(doc) << "\"\n";
  }
  return 0;
}
```

Output

```text
sentences = 3
  tokens [0, 5) label=sentence text="Dr. Ada wrote notes."
  tokens [5, 6) label=sentence text="Kofi"
  tokens [6, 8) label=sentence text="agreed!"
```

### Example 2

The two boundary rules, separately and together, on the same text. The table
prints the sentence count for each combination of `break_on_punct` and
`break_on_newline`, then the sentences produced by the default combination
of the two, so the reader can see which rule produced each boundary.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "Ada wrote notes. Kofi\nagreed! Mina left";
  for (const bool punct : {false, true}) {
    for (const bool newline : {false, true}) {
      nlp::SentencizerOptions options{.break_on_punct = punct, .break_on_newline = newline};
      nlp::Workflow wf{.lang = nlp::Language::English,
                       .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(options)}};
      const nlp::Doc doc = wf.infer(text);
      std::cout << "break_on_punct = " << (punct ? "true" : "false") << ", break_on_newline = "
                << (newline ? "true" : "false") << " -> sentences = " << doc.sentences.size() << "\n";
    }
  }

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const nlp::Doc doc = wf.infer(text);
  std::cout << "\ndefault sentences:\n";
  for (const nlp::Span& sentence : doc.sentences) {
    std::cout << "  \"" << sentence.text(doc) << "\"\n";
  }
  return 0;
}
```

Output

```text
break_on_punct = false, break_on_newline = false -> sentences = 1
break_on_punct = false, break_on_newline = true -> sentences = 2
break_on_punct = true, break_on_newline = false -> sentences = 3
break_on_punct = true, break_on_newline = true -> sentences = 4

default sentences:
  "Ada wrote notes."
  "Kofi"
  "agreed!"
  "Mina left"
```

### Example 3

The abbreviation rule on two inputs. With the option on, the period in `Dr.`
and in `etc.` does not end a sentence. With it off, every period does.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "Ask Dr. Ada for the report, etc. She can help.";
  for (const bool abbreviations : {true, false}) {
    nlp::SentencizerOptions options{.use_language_abbreviations = abbreviations};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(options)}};
    const nlp::Doc doc = wf.infer(text);

    std::cout << "use_language_abbreviations = " << (abbreviations ? "true" : "false")
              << " -> sentences = " << doc.sentences.size() << "\n";
    for (const nlp::Span& sentence : doc.sentences) {
      std::cout << "  \"" << sentence.text(doc) << "\"\n";
    }
  }
  return 0;
}
```

Output

```text
use_language_abbreviations = true -> sentences = 1
  "Ask Dr. Ada for the report, etc. She can help."
use_language_abbreviations = false -> sentences = 3
  "Ask Dr."
  "Ada for the report, etc."
  "She can help."
```

### Example 4

Punctuation breaks need punctuation tokens. The same two-sentence text is
tokenized twice: once with punctuation kept, which produces two sentences,
and once with punctuation dropped, which produces one. The second result is
not a sentencizer bug. The sentencizer never saw a period to break on.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "Ada wrote notes. Kofi agreed.";
  for (const bool keep : {true, false}) {
    nlp::RuleTokenizerOptions tokens{.keep_punct = keep};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(tokens), nlp::stage::sentencize()}};
    const nlp::Doc doc = wf.infer(text);

    std::cout << "keep_punct = " << (keep ? "true" : "false") << ", tokens = " << doc.tokens.size()
              << ", sentences = " << doc.sentences.size() << "\n";
  }
  return 0;
}
```

Output

```text
keep_punct = true, tokens = 7, sentences = 2
keep_punct = false, tokens = 5, sentences = 1
```

## See Also

- `stage::sentencize` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`RuleTokenizerStage`](RuleTokenizerStage.md) -- supplies the tokens and punctuation this stage reads.
- [`Doc::sentence_texts()`](../core/Doc/sentence_texts.md) -- returns the text of each sentence.
- [`WorkflowReport`](../workflow/WorkflowReport.md) -- reports sentence counts across a corpus.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
