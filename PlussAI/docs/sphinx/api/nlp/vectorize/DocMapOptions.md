# `pluss::ai::nlp::vectorize::DocMapOptions`

The two choices that decide what a document contributes to a feature matrix. Every
vectorizer in the family takes one of these, and so does [`describe`](describe.md), so
the same corpus described and then vectorized produces matching counts. The struct is
plain data built with designated initializers.

## Declaration

```cpp
struct pluss::ai::nlp::vectorize::DocMapOptions {
  bool use_lemmas = true;
  bool skip_punct = true;
};
```

## Fields

### `use_lemmas`

`bool`, default `true`. When `true`, a token contributes its lemma if the lemma is
non-empty, and its surface form otherwise. When `false`, every token contributes its
surface form, and lemmas are ignored.

The default is only useful when the pipeline has run a lemmatizer. Without one, every
lemma is empty and the option has no visible effect. With one, `wrote`, `writes`, and
`write` collapse into a single feature, which is usually what a bag-of-words model wants.
Turn it off when the inflection itself is the signal, such as tense in a classification
task.

### `skip_punct`

`bool`, default `true`. When `true`, tokens with `is_punct` set are excluded. A period
or a comma then never becomes a column, which keeps punctuation from dominating the
vocabulary of a corpus with many short sentences. When `false`, each punctuation mark is
a column like any word, which can help when the marks carry signal, such as question
marks in a question-detection task. A mapper built with these options keeps the marks
as terms, so the mapper's [`LexiconMapOptions::keep_punct`](../../classic/text/LexiconMapOptions.md)
is set from this field.

## Guidance

A document made only of punctuation produces no terms under `skip_punct`, and its row is
empty. A document with no tokens at all falls back to its raw text, which is the only
case where the fallback applies. Check the corpus with [`describe`](describe.md) before
trusting a fixed setting, because an empty row is easy to overlook when the rest of the
corpus looks plausible.

Use the same options for training and inference. Changing `use_lemmas` between the two
produces a feature space that does not match the one the model was fitted on.

## See Also

- [`CorpusStats`](CorpusStats.md) -- what [`describe`](describe.md) reports under these options.
- [`Mapper`](Mapper.md) -- consumes these options at construction.
- [`Hasher`](Hasher.md) -- consumes these options at construction.
- [`Token`](../core/Token.md) -- the `lemma`, `orth`, and `is_punct` fields these options read.

## Practical Examples

These examples show each option's effect on the feature text, then the combined
effect on a corpus summary.

### Example 1

Lemmas on and off. The same sentence produces different feature text, and the lemma
option decides whether inflected forms collapse. The lemmatizer runs in both cases, so
only the option differs.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Ada wrote books. Kofi writes books.");

  for (const bool lemmas : {true, false}) {
    nlp::vectorize::DocMapOptions options{.use_lemmas = lemmas};
    std::string text;
    for (const nlp::Token& token : doc.tokens) {
      if (options.skip_punct && token.is_punct) continue;
      const std::string& piece = (options.use_lemmas && !token.lemma.empty()) ? token.lemma : token.orth;
      if (!text.empty()) text += ' ';
      text += piece;
    }
    std::cout << "use_lemmas = " << (lemmas ? "true" : "false") << ": " << text << "\n";
  }
  return 0;
}
```

Output

```text
use_lemmas = true: ada write book kofi write book
use_lemmas = false: Ada wrote books Kofi writes books
```

### Example 2

Punctuation on and off. With `skip_punct` on, the commas and periods are gone from the
feature text, and with it off they stay as features.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Ada wrote, then left.");

  for (const bool skip : {true, false}) {
    nlp::vectorize::DocMapOptions options{.skip_punct = skip};
    std::string text;
    for (const nlp::Token& token : doc.tokens) {
      if (options.skip_punct && token.is_punct) continue;
      if (!text.empty()) text += ' ';
      text += token.lemma.empty() ? token.orth : token.lemma;
    }
    std::cout << "skip_punct = " << (skip ? "true" : "false") << ": " << text << "\n";
  }
  return 0;
}
```

Output

```text
skip_punct = true: ada write then left
skip_punct = false: ada write , then left .
```

### Example 3

The fallback rule and the punctuation option together. An untokenized document has no
tokens, so its raw text supplies the terms. A document made only of punctuation
contributes its marks when `skip_punct` is off, and nothing when it is on.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc untokenized{};
  untokenized.text = "Ada writes books.";
  const auto marks = wf.infer(std::vector<std::string>{"...!?"});

  for (const bool skip : {true, false}) {
    const nlp::vectorize::DocMapOptions options{.skip_punct = skip};
    const auto raw = nlp::vectorize::describe(std::vector<nlp::Doc>{untokenized}, options);
    const auto punct = nlp::vectorize::describe(marks, options);
    std::cout << "skip_punct = " << (skip ? "true" : "false") << ": untokenized terms = " << raw.token_count
              << ", punctuation-only terms = " << punct.token_count << "\n";
  }
  return 0;
}
```

Output

```text
skip_punct = true: untokenized terms = 3, punctuation-only terms = 0
skip_punct = false: untokenized terms = 4, punctuation-only terms = 5
```

## Changelog

- **0.1** -- initial documented surface.
