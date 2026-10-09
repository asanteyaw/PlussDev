# `pluss::ai::nlp::vectorize::describe`

Summarizes a corpus without building a feature matrix. It counts terms with the rule the mappers
use, counts documents, sentences, and phrases, and returns the distinct terms with their document
frequencies. Use it to choose `max_features` and `min_document_frequency` before learning, or to
check a corpus for preparation problems.

## Signature

```cpp
pluss::ai::nlp::vectorize::CorpusStats pluss::ai::nlp::vectorize::describe(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    pluss::ai::nlp::vectorize::DocMapOptions options = {});
```

## What It Does

Builds each document's feature text under `options`, and splits that text into terms the way a
mapper does. For each document it records the sentences and phrases the document carries, the
number of terms, and the number of distinct terms. Across the corpus it counts the documents that
contain each term, and ranks the terms by that count, most frequent first, with alphabetical ties.
The ranking is what the result's `vocabulary` and `document_frequency` report, so the summary
previews the order a `Mapper` would learn.

Terms are lowercased, as the default mapper options are, so the vocabulary lists `ada` where the
surface text was `Ada`. A mapper built with `lowercase = false` keeps case, and its vocabulary can
then be larger than the summary reports. The function counts unigrams only, so a mapper built with
n-grams has more columns than the summary's `unique_term_count`.

The function runs no stage, so the documents must already carry the annotations you want counted.
`sentence_count` and `phrase_count` are zero for documents that never ran a sentencizer or a
syntax stage, and the summary does not fill them in. It does not change the documents.

## Parameters

`docs` is the corpus, as a vector of prepared documents. An empty vector is valid and yields an
all-zero summary.

`options` is the [`DocMapOptions`](DocMapOptions.md) that decides which tokens become terms and
whether punctuation counts. Use the same options the vectorizer will use, so the summary reflects
its input.

## Returns

A [`CorpusStats`](CorpusStats.md) value. Its fields are corpus totals, per-document series, the
ranked vocabulary, and the matching document frequencies.

## Errors

None. The function does not fail on any document, including empty ones, which contribute zero to
every total.

## Practical Examples

These examples summarize a corpus before learning, show how the punctuation option changes the
counts, and use the summary to choose a minimum document frequency.

### Example 1

Summarizing a corpus before learning. The vocabulary and the document frequency tell you how many
terms are shared and how many appear only once.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{
      "Ada writes APIs.", "Kofi writes APIs for NASA.", "Mina reads books.", "Zed sings."});
  const nlp::vectorize::CorpusStats stats = nlp::vectorize::describe(docs);

  std::cout << "documents = " << stats.doc_count << ", tokens = " << stats.token_count
            << ", distinct terms = " << stats.unique_term_count << "\n";
  const auto frequencies = stats.document_frequency.to(pluss::pCPU, pluss::ScalarType::Float64)
                               .contiguous().template to_vec<double>();
  std::int64_t singletons = 0;
  for (const double frequency : frequencies) {
    if (frequency == 1.0) ++singletons;
  }
  std::cout << "terms in a single document = " << singletons << "\n";
  return 0;
}
```

Output

```text
documents = 4, tokens = 13, distinct terms = 11
terms in a single document = 9
```

### Example 2

The effect of the punctuation option on the summary. With punctuation skipped, the period and comma
terms are absent; with it kept, they add to the term total and to the vocabulary.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada wrote, then left.", "Kofi left."});

  for (const bool skip : {true, false}) {
    const nlp::vectorize::CorpusStats stats =
        nlp::vectorize::describe(docs, nlp::vectorize::DocMapOptions{.skip_punct = skip});
    std::cout << "skip_punct = " << (skip ? "true" : "false") << ": token_count = " << stats.token_count
              << ", unique_term_count = " << stats.unique_term_count << "\n";
  }
  return 0;
}
```

Output

```text
skip_punct = true: token_count = 6, unique_term_count = 5
skip_punct = false: token_count = 9, unique_term_count = 7
```

### Example 3

Using the summary to choose a minimum document frequency. The program counts how many terms each
threshold would keep, so the choice is made from the corpus rather than guessed.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{
      "Ada writes APIs.", "Kofi writes APIs.", "Mina writes books.", "Zed sings."});
  const nlp::vectorize::CorpusStats stats = nlp::vectorize::describe(docs);

  const auto frequencies = stats.document_frequency.to(pluss::pCPU, pluss::ScalarType::Float64)
                               .contiguous().template to_vec<double>();
  for (const std::int64_t threshold : {1, 2, 3}) {
    std::int64_t kept = 0;
    for (const double frequency : frequencies) {
      if (frequency >= static_cast<double>(threshold)) ++kept;
    }
    std::cout << "min_document_frequency = " << threshold << " keeps " << kept << " term(s)\n";
  }
  return 0;
}
```

Output

```text
min_document_frequency = 1 keeps 8 term(s)
min_document_frequency = 2 keeps 2 term(s)
min_document_frequency = 3 keeps 1 term(s)
```

## See Also

- [`CorpusStats`](CorpusStats.md) -- the result this function returns.
- [`DocMapOptions`](DocMapOptions.md) -- the rules the counts follow.
- [`Mapper`](Mapper.md) -- learns the vocabulary this summary previews.

## Changelog

- **0.1** -- initial documented surface.
