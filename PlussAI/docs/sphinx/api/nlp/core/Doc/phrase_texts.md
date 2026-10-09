<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 21fb9ba24816 -->
# `pluss::ai::nlp::Doc::phrase_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> phrase_texts() const;
```

## What It Does

Returns each entry of `phrases` as its sliced substring of `text`.
`phrases` is only populated by [`SyntaxStage`](../../tasks/SyntaxStage.md)
-- it holds the noun-chunk-style branches the dependency parse groups
together (a head token plus its attached dependents, such as a
multi-word proper noun), not every possible subtree.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per detected phrase.

## Errors

None.

## Practical Examples

### Example 1

A sentence with two multi-word proper-noun phrases.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize(),
                 nlp::stage::tagger(), nlp::stage::syntax()},
  };
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs for NASA in New York.");

  std::cout << "count=" << doc.phrase_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=4
```

### Example 2

Reading every phrase shows the title-plus-name and the two-word place
name grouped as single units.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize(), nlp::stage::lemmatize(),
                 nlp::stage::tagger(), nlp::stage::syntax()},
  };
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs for NASA in New York.");

  for (const std::string& phrase : doc.phrase_texts()) {
    std::cout << "<" << phrase << ">";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
<Dr. Laurel><APIs><NASA><New York>
```

### Example 3

Without a syntax stage, there is no dependency structure to group
phrases from.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs for NASA in New York.");

  std::cout << "count=" << doc.phrase_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`SyntaxStage`](../../tasks/SyntaxStage.md) -- what populates
  `phrases`.
- [`branch_of`](branch_of.md) -- computing the same kind of span for
  an arbitrary token, not just the ones `SyntaxStage` grouped.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maybe_lexicon.html">
    <span class="label">Previous</span>
    <span class="title">maybe_lexicon</span>
  </a>
  <a class="next-link" href="right_edge_of.html">
    <span class="label">Next</span>
    <span class="title">right_edge_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
