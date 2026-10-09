<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3deaf357f135 -->
# `pluss::ai::nlp::Doc::sentence_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> sentence_texts() const;
```

## What It Does

Returns each entry of `sentences` as its sliced substring of `text`,
via [`Span::text`](../Span.md). `sentences` is only populated by
[`SentencizerStage`](../../tasks/SentencizerStage.md) -- without it in
the pipeline, this is always empty, regardless of how many tokens the
`Doc` has.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per detected sentence.

## Errors

None.

## Practical Examples

### Example 1

Two sentences, split on the period.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs. Second sentence here.");

  std::cout << "count=" << doc.sentence_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=2
```

### Example 2

The sentencizer consults the active `Language`'s abbreviation list, so
"Dr." does not end the sentence early.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()}};
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs. Second sentence!");

  for (const std::string& sentence : doc.sentence_texts()) {
    std::cout << "{" << sentence << "}";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
{Dr. Laurel writes APIs.}{Second sentence!}
```

### Example 3

Without a sentencizer stage, there are no sentence boundaries to read.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs. Second sentence here.");

  std::cout << "count=" << doc.sentence_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`SentencizerStage`](../../tasks/SentencizerStage.md) -- what
  populates `sentences`.
- [`Span::text`](../Span.md) -- the slicing this delegates to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="right_edge_of.html">
    <span class="label">Previous</span>
    <span class="title">right_edge_of</span>
  </a>
  <a class="next-link" href="token_texts.html">
    <span class="label">Next</span>
    <span class="title">token_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
