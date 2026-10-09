# `pluss::ai::nlp::assess::ner`

Scores predicted entity spans against gold spans across a corpus and returns an
[`EntityScore`](EntityScore.md) with precision, recall, and F1. A predicted entity
counts as correct only when a gold entity has the same token range and the same label.
Each gold entity can be matched once, so a prediction that duplicates a correct span
does not earn a second credit.

As with the tag scorer, the two documents in each pair must align token for token. The
function checks the alignment before it compares any entities, and it raises an error
rather than scoring documents that describe different text.

## Signature

```cpp
pluss::ai::nlp::EntityScore pluss::ai::nlp::assess::ner(
    const std::vector<pluss::ai::nlp::Doc>& predicted,
    const std::vector<pluss::ai::nlp::Doc>& gold);
```

## What It Does

Walks the document pairs in order and verifies that each pair aligns. For each pair it
counts the predicted entities and the gold entities, then matches them one to one. A
predicted entity matches a gold entity when their token starts, token ends, and labels
are all equal. Each gold entity can satisfy at most one prediction, so duplicates in the
prediction are counts of error, not of success.

Precision is the number of correct predictions divided by the number of predictions.
Recall is the number of correct predictions divided by the number of gold entities. F1
is their harmonic mean, which is zero when either one is zero. Each ratio is zero when
its denominator is zero, so a corpus with no predictions has precision zero, not an
undefined value.

## Parameters

`predicted` holds the documents with their entities in `Doc::entities`, one document per
input text. It must have the same length as `gold`, and each pair must align token for
token.

`gold` holds the reference documents with the same alignment. Only the entity spans and
the alignment fields are read, so the gold documents may carry other annotations.

## Returns

An `EntityScore`. Its `doc_count`, `gold_count`, `predicted_count`, and `correct_count`
are corpus totals, and its `precision`, `recall`, and `f1` are computed from those totals.
The struct is described on its own page.

## Errors

- The two vectors have different lengths: the document counts must match.
- A document pair has different token counts, or a token differs in character span or
  surface text: the token sequences must align exactly.

## Practical Examples

These examples score entity output against a reference, show the one-to-one matching
rule with a wrong label and a duplicated span, and show the empty-prediction case.

### Example 1

A prediction with one correct entity, one entity whose label is wrong, and one gold
entity that it misses. The output shows the three counts and the three ratios that follow from them.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc gold = wf.infer("Kofi visits Tema from Accra.");
  gold.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                   {.token_start = 2, .token_end = 3, .label = "GPE"},
                   {.token_start = 4, .token_end = 5, .label = "GPE"}};

  nlp::Doc predicted = wf.infer("Kofi visits Tema from Accra.");
  predicted.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                        {.token_start = 2, .token_end = 3, .label = "ORG"}};

  const nlp::EntityScore score = nlp::assess::ner({predicted}, {gold});
  std::cout << "gold = " << score.gold_count << ", predicted = " << score.predicted_count
            << ", correct = " << score.correct_count << "\n";
  std::cout << "precision = " << score.precision << ", recall = " << score.recall
            << ", f1 = " << score.f1 << "\n";
  return 0;
}
```

Output

```text
gold = 3, predicted = 2, correct = 1
precision = 0.5, recall = 0.333333, f1 = 0.4
```

### Example 2

Matching is one to one. The prediction contains the correct span twice, but the gold
document has it once, so only one prediction counts as correct and the duplicate lowers
precision.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc gold = wf.infer("Kofi visits Tema.");
  gold.entities = {{.token_start = 2, .token_end = 3, .label = "GPE"}};

  nlp::Doc predicted = wf.infer("Kofi visits Tema.");
  predicted.entities = {{.token_start = 2, .token_end = 3, .label = "GPE"},
                        {.token_start = 2, .token_end = 3, .label = "GPE"}};

  const nlp::EntityScore score = nlp::assess::ner({predicted}, {gold});
  std::cout << "correct = " << score.correct_count << " of " << score.predicted_count << " predictions\n";
  std::cout << "precision = " << score.precision << ", recall = " << score.recall << "\n";
  return 0;
}
```

Output

```text
correct = 1 of 2 predictions
precision = 0.5, recall = 1
```

### Example 3

The empty cases. A corpus with gold entities and no predictions has zero precision
and zero recall, and an F1 of zero, because the ratios are defined as zero rather than
undefined. The program prints the three values for that corpus.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc gold = wf.infer("Kofi visits Tema.");
  gold.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}};
  const nlp::Doc predicted = wf.infer("Kofi visits Tema.");

  const nlp::EntityScore score = nlp::assess::ner({predicted}, {gold});
  std::cout << "predicted = " << score.predicted_count << ", gold = " << score.gold_count << "\n";
  std::cout << "precision = " << score.precision << ", recall = " << score.recall << ", f1 = " << score.f1 << "\n";
  return 0;
}
```

Output

```text
predicted = 0, gold = 1
precision = 0, recall = 0, f1 = 0
```

## See Also

- [`EntityScore`](EntityScore.md) -- the result this function returns.
- [`NerStage`](NerStage.md) -- produces the predicted entities this function scores.
- [`assess::tagger`](assess_tagger.md) -- the token-level counterpart.

## Changelog

- **0.1** -- initial documented surface.
