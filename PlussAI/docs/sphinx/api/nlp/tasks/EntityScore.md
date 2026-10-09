# `pluss::ai::nlp::EntityScore`

The result of [`assess::ner`](assess_ner.md): the counts behind entity precision,
recall, and F1. Like `TagScore`, it is plain data, and every field is a corpus total or a
ratio computed from those totals.

The counts record what the scorer saw. `gold_count` is how many entities the reference
contains, `predicted_count` is how many the system produced, and `correct_count` is how
many predictions matched a gold entity exactly. The three ratios follow from those
counts, so the struct can be checked by hand.

## Fields

### `doc_count`

`std::int64_t`, default `0`. The number of document pairs scored.

### `gold_count`

`std::int64_t`, default `0`. The total number of entities in the gold documents. It is
the recall denominator.

### `predicted_count`

`std::int64_t`, default `0`. The total number of entities in the predicted documents. It
is the precision denominator.

### `correct_count`

`std::int64_t`, default `0`. The number of predictions that matched a distinct gold entity
on token range and label. Each gold entity contributes at most one correct prediction.

### `precision`

`double`, default `0.0`. `correct_count` divided by `predicted_count`, or `0.0` when the
system predicted nothing. Precision answers how much of what the system said was right.

### `recall`

`double`, default `0.0`. `correct_count` divided by `gold_count`, or `0.0` when the
reference has no entities. Recall answers how much of what should have been found was
found.

### `f1`

`double`, default `0.0`. The harmonic mean of precision and recall,
`2 * precision * recall / (precision + recall)`, and `0.0` when both are zero. F1 is
high only when both precision and recall are high, so a system that predicts everything
scores poorly on it.

## Guidance

Report all three ratios together. A high precision with a low recall is a system that
is careful and misses things, and the reverse is a system that is thorough and noisy.
F1 hides that difference, so it is a summary, not a diagnosis.

The counts also show whether a ratio is trustworthy. A precision of one over a single
prediction says almost nothing, and the same ratio over a thousand predictions is strong
evidence. Read `predicted_count` and `gold_count` before reading the ratios.

To combine runs, sum the counts and recompute the ratios from the sums, as with
`TagScore`.

## See Also

- [`assess::ner`](assess_ner.md) -- produces this struct.
- [`NerStage`](NerStage.md) -- produces the predicted entities.
- [`TagScore`](TagScore.md) -- the token-level counterpart.

## Practical Examples

These examples read a score's fields, show how the three ratios differ on the same
counts, and combine two batches into one corpus figure.

### Example 1

A score printed field by field for a corpus of one document, with the ratios
derived from the counts shown next to them, so the arithmetic can be followed.

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
  std::cout << "doc_count = " << score.doc_count << "\n";
  std::cout << "gold_count = " << score.gold_count << "\n";
  std::cout << "predicted_count = " << score.predicted_count << "\n";
  std::cout << "correct_count = " << score.correct_count << "\n";
  std::cout << "precision = " << score.correct_count << " / " << score.predicted_count << " = " << score.precision << "\n";
  std::cout << "recall = " << score.correct_count << " / " << score.gold_count << " = " << score.recall << "\n";
  std::cout << "f1 = " << score.f1 << "\n";
  return 0;
}
```

Output

```text
doc_count = 1
gold_count = 3
predicted_count = 2
correct_count = 1
precision = 1 / 2 = 0.5
recall = 1 / 3 = 0.333333
f1 = 0.4
```

### Example 2

Three systems with the same F1 and different precision and recall. The gold set has
four entities. The careful system finds two of them with two predictions, the balanced
system finds three of them with five predictions, and the noisy system finds all four
with eight predictions. Each has an F1 of two thirds, but their error profiles differ,
which is why the two ratios are reported together.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const char* text = "Kofi visits Tema from Accra today.";
  nlp::Doc gold = wf.infer(text);
  gold.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                   {.token_start = 2, .token_end = 3, .label = "GPE"},
                   {.token_start = 4, .token_end = 5, .label = "GPE"},
                   {.token_start = 5, .token_end = 6, .label = "DATE"}};

  nlp::Doc careful = wf.infer(text);
  careful.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                      {.token_start = 2, .token_end = 3, .label = "GPE"}};

  nlp::Doc balanced = wf.infer(text);
  balanced.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                       {.token_start = 2, .token_end = 3, .label = "GPE"},
                       {.token_start = 4, .token_end = 5, .label = "GPE"},
                       {.token_start = 1, .token_end = 2, .label = "PERSON"},
                       {.token_start = 3, .token_end = 4, .label = "ORG"}};

  nlp::Doc noisy = wf.infer(text);
  noisy.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                    {.token_start = 2, .token_end = 3, .label = "GPE"},
                    {.token_start = 4, .token_end = 5, .label = "GPE"},
                    {.token_start = 5, .token_end = 6, .label = "DATE"},
                    {.token_start = 1, .token_end = 2, .label = "PERSON"},
                    {.token_start = 3, .token_end = 4, .label = "ORG"},
                    {.token_start = 0, .token_end = 2, .label = "ORG"},
                    {.token_start = 2, .token_end = 4, .label = "GPE"}};

  const std::pair<const char*, nlp::Doc*> systems[] = {
      {"careful", &careful}, {"balanced", &balanced}, {"noisy", &noisy}};
  for (const auto& [name, predicted] : systems) {
    const nlp::EntityScore score = nlp::assess::ner({*predicted}, {gold});
    std::cout << name << ": precision = " << score.precision << ", recall = " << score.recall
              << ", f1 = " << score.f1 << "\n";
  }
  return 0;
}
```

Output

```text
careful: precision = 1, recall = 0.5, f1 = 0.666667
balanced: precision = 0.6, recall = 0.75, f1 = 0.666667
noisy: precision = 0.5, recall = 1, f1 = 0.666667
```

### Example 3

Combining two batches. The counts are summed and the ratios recomputed from the
sums, which is the only way to get the corpus figure when the batches differ in size.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};

  nlp::Doc first_gold = wf.infer("Kofi visits Tema.");
  first_gold.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}};
  nlp::Doc first_pred = wf.infer("Kofi visits Tema.");
  first_pred.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"}};

  nlp::Doc second_gold = wf.infer("Ada visits Accra from Tema today.");
  second_gold.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                          {.token_start = 2, .token_end = 3, .label = "GPE"},
                          {.token_start = 4, .token_end = 5, .label = "GPE"}};
  nlp::Doc second_pred = wf.infer("Ada visits Accra from Tema today.");
  second_pred.entities = {{.token_start = 0, .token_end = 1, .label = "PERSON"},
                          {.token_start = 2, .token_end = 3, .label = "GPE"}};

  const nlp::EntityScore a = nlp::assess::ner({first_pred}, {first_gold});
  const nlp::EntityScore b = nlp::assess::ner({second_pred}, {second_gold});

  nlp::EntityScore total{};
  total.doc_count = a.doc_count + b.doc_count;
  total.gold_count = a.gold_count + b.gold_count;
  total.predicted_count = a.predicted_count + b.predicted_count;
  total.correct_count = a.correct_count + b.correct_count;
  total.precision = static_cast<double>(total.correct_count) / static_cast<double>(total.predicted_count);
  total.recall = static_cast<double>(total.correct_count) / static_cast<double>(total.gold_count);
  total.f1 = 2.0 * total.precision * total.recall / (total.precision + total.recall);

  std::cout << "documents = " << total.doc_count << ", gold = " << total.gold_count
            << ", predicted = " << total.predicted_count << ", correct = " << total.correct_count << "\n";
  std::cout << "precision = " << total.precision << ", recall = " << total.recall << ", f1 = " << total.f1 << "\n";
  return 0;
}
```

Output

```text
documents = 2, gold = 4, predicted = 3, correct = 3
precision = 1, recall = 0.75, f1 = 0.857143
```

## Changelog

- **0.1** -- initial documented surface.
