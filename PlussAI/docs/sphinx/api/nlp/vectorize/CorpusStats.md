# `pluss::ai::nlp::vectorize::CorpusStats`

The corpus summary [`describe`](describe.md) returns. It counts documents, tokens, and
sentences, measures how many distinct terms the corpus has and how many documents each
appears in, and carries per-document counts as series so they can be inspected or plotted.
Every count follows the [`DocMapOptions`](DocMapOptions.md) it was described with, so the
summary matches what a mapper would count.

## Declaration

```cpp
struct pluss::ai::nlp::vectorize::CorpusStats {
  std::int64_t doc_count = 0;
  std::int64_t token_count = 0;
  std::int64_t sentence_count = 0;
  std::int64_t phrase_count = 0;
  std::int64_t unique_term_count = 0;
  pluss::Series document_token_counts;
  pluss::Series document_unique_term_counts;
  pluss::Note vocabulary;
  pluss::Series document_frequency;
};
```

## Fields

### `doc_count`

`std::int64_t`, default `0`. The number of documents described. It counts empty documents
too, so it can exceed the number of documents that contributed any term.

### `token_count`

`std::int64_t`, default `0`. The total number of terms across the corpus, counted the way a
mapper counts them. The count is a term count, not a token count: a multi-word token is
split, and a punctuation mark counts only when the options keep punctuation.

### `sentence_count`

`std::int64_t`, default `0`. The sum of `Doc::sentences` across the corpus. It is zero for a
corpus that never ran a sentencizer, and the summary does not infer sentences itself.

### `phrase_count`

`std::int64_t`, default `0`. The sum of `Doc::phrases` across the corpus. It is zero when no
syntax stage ran.

### `unique_term_count`

`std::int64_t`, default `0`. The number of distinct terms in the corpus. It is the length of
`vocabulary` and of `document_frequency`.

### `document_token_counts`

`pluss::Series`, undefined by default. One value per document: the number of terms that
document contributed.

### `document_unique_term_counts`

`pluss::Series`, undefined by default. One value per document: the number of distinct terms
in that document. Its ratio to `document_token_counts` measures how repetitive each document is.

### `vocabulary`

`pluss::Note`, undefined by default. Every distinct term, lowercased, ordered by document
frequency, most frequent first, with ties broken alphabetically. The order matches the order a
[`Mapper`](Mapper.md) learns its vocabulary in, before any `max_features` cap.

### `document_frequency`

`pluss::Series`, undefined by default. The number of documents each term appears in, aligned
index for index with `vocabulary`. It is the quantity `min_document_frequency` filters on.

## Guidance

Read `unique_term_count` against `token_count` for a rough sense of how varied the corpus is.
Read `document_frequency` for the tail: a large number of terms that appear in a single
document is the signal to raise `min_document_frequency`, which removes them before they inflate
the feature width.

The per-document series are the place to look for outliers. A document with a very high
`document_token_counts` value, or one with almost no terms, usually reflects a preparation
problem rather than real content.

## See Also

- [`describe`](describe.md) -- produces this struct.
- [`DocMapOptions`](DocMapOptions.md) -- the rules every count follows.
- [`Mapper`](Mapper.md) -- learns the vocabulary this summary previews.

## Practical Examples

These examples read the summary of a small corpus, check the vocabulary order against the
document frequencies, and show the per-document series.

### Example 1

A summary of three documents, printed field by field, so each count can be checked against the
text.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat.", "The dog sat.", "A cat ran!"});
  const nlp::vectorize::CorpusStats stats = nlp::vectorize::describe(docs);

  std::cout << "doc_count = " << stats.doc_count << "\n";
  std::cout << "token_count = " << stats.token_count << "\n";
  std::cout << "unique_term_count = " << stats.unique_term_count << "\n";
  std::cout << "sentence_count = " << stats.sentence_count << "\n";
  return 0;
}
```

Output

```text
doc_count = 3
token_count = 9
unique_term_count = 6
sentence_count = 0
```

### Example 2

The vocabulary and its document frequencies, read side by side. The order is by frequency first,
then alphabetically, so the most widely shared terms lead.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat.", "The dog sat.", "A cat ran!"});
  const nlp::vectorize::CorpusStats stats = nlp::vectorize::describe(docs);

  const auto frequencies = stats.document_frequency.to(pluss::pCPU, pluss::ScalarType::Float64)
                               .contiguous().template host<double>();
  for (std::size_t i = 0; i < frequencies.size(); ++i) {
    std::cout << stats.vocabulary.at(static_cast<std::int64_t>(i)) << ": in " << frequencies[i] << " document(s)\n";
  }
  return 0;
}
```

Output

```text
cat: in 2 document(s)
sat: in 2 document(s)
the: in 2 document(s)
a: in 1 document(s)
dog: in 1 document(s)
ran: in 1 document(s)
```

### Example 3

The per-document series. The summary reports, for each document, how many terms it contributed and
how many of those were distinct, which exposes repetition.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat cat cat dog", "bird fish"});
  const nlp::vectorize::CorpusStats stats = nlp::vectorize::describe(docs);

  const auto terms = stats.document_token_counts.to(pluss::pCPU, pluss::ScalarType::Float64)
                         .contiguous().template host<double>();
  const auto distinct = stats.document_unique_term_counts.to(pluss::pCPU, pluss::ScalarType::Float64)
                            .contiguous().template host<double>();
  for (std::size_t i = 0; i < terms.size(); ++i) {
    std::cout << "document " << i << ": terms = " << terms[i] << ", distinct = " << distinct[i] << "\n";
  }
  return 0;
}
```

Output

```text
document 0: terms = 4, distinct = 2
document 1: terms = 2, distinct = 2
```

## Changelog

- **0.1** -- initial documented surface.
