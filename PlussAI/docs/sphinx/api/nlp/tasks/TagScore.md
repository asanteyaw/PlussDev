# `pluss::ai::nlp::TagScore`

The result of [`assess::tagger`](assess_tagger.md): the counts behind a
part-of-speech accuracy figure, and the accuracy itself. It is plain data. The
function fills it, and the caller reads its fields.

Every field is a corpus total, not a per-document figure. The counts can be added
across separate corpora, and the accuracy recomputed from the sums, which is how a
score from several batches is combined into one.

## Fields

### `doc_count`

`std::int64_t`, default `0`. The number of document pairs the function scored. A
corpus of ten documents has a `doc_count` of ten, whatever their lengths.

### `token_count`

`std::int64_t`, default `0`. The total number of tokens across every scored document.
It is the denominator of the accuracy, so it is also the number of tags that were
compared.

### `correct_count`

`std::int64_t`, default `0`. The number of tokens whose predicted tag equals the gold
tag. Two empty tags are equal, so a token that neither side tagged is counted here.

### `accuracy`

`double`, default `0.0`. `correct_count` divided by `token_count`. It is `0.0` when
the corpus has no tokens, which is the only case where the division would otherwise
fail.

## Guidance

Read `accuracy` for a single number, and read the counts when you need to know how
much evidence stands behind it. An accuracy of `0.9` over twelve tokens is a weak
claim, and the same value over twelve thousand tokens is a strong one.

To combine scores from several runs, sum `doc_count`, `token_count`, and
`correct_count` across the runs, and divide the sums. Averaging the accuracies
instead weights each run equally regardless of its size, which usually gives a
different and less accurate figure.

## See Also

- [`assess::tagger`](assess_tagger.md) -- produces this struct.
- [`EntityScore`](EntityScore.md) -- the entity-level counterpart.

## Practical Examples

These examples read the fields of a score, combine two batches into one corpus
figure, and show why the combined accuracy is not the average of the batch
accuracies.

### Example 1

A score for one sentence, printed field by field, so each count can be checked
against the sentence.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc gold = base.infer("Ada writes books.");
  nlp::Doc predicted = base.infer("Ada writes books.");
  const char* gold_tags[] = {"PROPN", "VERB", "NOUN", "PUNCT"};
  const char* predicted_tags[] = {"PROPN", "NOUN", "NOUN", "PUNCT"};
  for (std::size_t i = 0; i < gold.tokens.size(); ++i) {
    gold.tokens[i].pos = gold_tags[i];
    predicted.tokens[i].pos = predicted_tags[i];
  }

  const nlp::TagScore score = nlp::assess::tagger({predicted}, {gold});
  std::cout << "doc_count = " << score.doc_count << "\n";
  std::cout << "token_count = " << score.token_count << "\n";
  std::cout << "correct_count = " << score.correct_count << "\n";
  std::cout << "accuracy = " << score.accuracy << "\n";
  return 0;
}
```

Output

```text
doc_count = 1
token_count = 4
correct_count = 3
accuracy = 0.75
```

### Example 2

Combining two batches by summing their counts. The sums give the corpus accuracy, and
the program prints it next to the mean of the two batch accuracies to show the
difference when the batches differ in size.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow base{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc small_gold = base.infer("Stop now.");
  nlp::Doc small_pred = small_gold;
  for (nlp::Token& token : small_gold.tokens) token.pos = "VERB";
  for (nlp::Token& token : small_pred.tokens) token.pos = "VERB";

  nlp::Doc large_gold = base.infer("Ada reads the long report carefully every morning.");
  nlp::Doc large_pred = large_gold;
  for (nlp::Token& token : large_gold.tokens) token.pos = "NOUN";
  for (nlp::Token& token : large_pred.tokens) token.pos = "VERB";

  const nlp::TagScore a = nlp::assess::tagger({small_pred}, {small_gold});
  const nlp::TagScore b = nlp::assess::tagger({large_pred}, {large_gold});

  nlp::TagScore combined{};
  combined.doc_count = a.doc_count + b.doc_count;
  combined.token_count = a.token_count + b.token_count;
  combined.correct_count = a.correct_count + b.correct_count;
  combined.accuracy = static_cast<double>(combined.correct_count) / static_cast<double>(combined.token_count);

  std::cout << "combined accuracy = " << combined.accuracy << " over " << combined.token_count << " tokens\n";
  std::cout << "mean of batch accuracies = " << (a.accuracy + b.accuracy) / 2.0 << "\n";
  return 0;
}
```

Output

```text
combined accuracy = 0.25 over 12 tokens
mean of batch accuracies = 0.5
```

### Example 3

The empty corpus. A score over no documents has zero counts and an accuracy of zero,
so a caller can detect the empty case from `token_count` rather than from a division
error.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::TagScore score = nlp::assess::tagger({}, {});
  std::cout << "doc_count = " << score.doc_count << ", token_count = " << score.token_count
            << ", accuracy = " << score.accuracy << "\n";
  return 0;
}
```

Output

```text
doc_count = 0, token_count = 0, accuracy = 0
```

## Changelog

- **0.1** -- initial documented surface.
