<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 880dc75ed471 -->
# `pluss::ai::nlp::Doc::entity_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> entity_texts() const;
```

## What It Does

Returns each entry of `entities` as its sliced substring of `text`.
`entities` is only populated by [`NerStage`](../../tasks/NerStage.md)
-- via rule-based [`EntityPattern`](../../tasks/EntityPattern.md)
matches, a trained model, or both together, depending on how the stage
was configured. Each `Span` in `entities` also carries its own
`label` (the entity type, such as `"PERSON"` or `"ORG"`), readable
directly from `Doc::entities` when the text alone is not enough.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per detected entity.

## Errors

None.

## Practical Examples

### Example 1

One rule-based entity pattern.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::ner({nlp::EntityPattern{.label = "ORG", .tokens = {"nasa"}, .use_lemmas = true}})},
  };
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA.");

  std::cout << "count=" << doc.entity_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=1
```

### Example 2

Two patterns, with each entity's own label read directly from
`Doc::entities` alongside its text.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::ner({
                     nlp::EntityPattern{.label = "PERSON", .tokens = {"laurel"}, .use_lemmas = true},
                     nlp::EntityPattern{.label = "ORG", .tokens = {"nasa"}, .use_lemmas = true},
                 })},
  };
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA.");

  for (const nlp::Span& entity : doc.entities) {
    std::cout << entity.label << ":" << entity.text(doc) << " ";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
PERSON:Laurel ORG:NASA 
```

### Example 3

Without an NER stage, there are no entities to read.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA.");

  std::cout << "count=" << doc.entity_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`NerStage`](../../tasks/NerStage.md) -- what populates `entities`.
- [`match_texts`](match_texts.md) -- the analogous accessor for
  [`MatcherStage`](../../tasks/MatcherStage.md)'s own matches.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ensure_lexicon.html">
    <span class="label">Previous</span>
    <span class="title">ensure_lexicon</span>
  </a>
  <a class="next-link" href="left_edge_of.html">
    <span class="label">Next</span>
    <span class="title">left_edge_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
