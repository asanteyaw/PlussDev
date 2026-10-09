<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6061475964da -->
# `pluss::ai::nlp::Doc::match_texts`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::string> match_texts() const;
```

## What It Does

Returns each entry of `matches` as its sliced substring of `text`.
`matches` is only populated by [`MatcherStage`](../../tasks/MatcherStage.md)
-- one entry per [`MatcherPattern`](../../tasks/MatcherPattern.md) that
hit, in the order the matcher found them (not necessarily the order
the patterns were registered in).

## Parameters

None.

## Returns

A `std::vector<std::string>`, one entry per match.

## Errors

None.

## Practical Examples

### Example 1

One lemma-based pattern matching an inflected phrase.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::matcher({nlp::MatcherPattern{.label = "action", .tokens = {"write", "api"}, .use_lemmas = true}})},
  };
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  std::cout << "count=" << doc.match_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=1
```

### Example 2

Two patterns -- one on lemmas, one on exact surface form -- both hit
in the same document.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::matcher({
                     nlp::MatcherPattern{.label = "lang", .tokens = {"c++"}, .use_lemmas = false},
                     nlp::MatcherPattern{.label = "action", .tokens = {"write", "api"}, .use_lemmas = true},
                 })},
  };
  const nlp::Doc doc = wf.infer("Laurel writes APIs in C++.");

  for (const std::string& match : doc.match_texts()) {
    std::cout << "[" << match << "]";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
[writes APIs][C++]
```

### Example 3

Without a matcher stage, nothing is ever recorded.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  std::cout << "count=" << doc.match_texts().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`MatcherStage`](../../tasks/MatcherStage.md) -- what populates
  `matches`.
- [`entity_texts`](entity_texts.md) -- the analogous accessor for
  [`NerStage`](../../tasks/NerStage.md)'s own entity spans.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lineage_of.html">
    <span class="label">Previous</span>
    <span class="title">lineage_of</span>
  </a>
  <a class="next-link" href="maybe_lexicon.html">
    <span class="label">Next</span>
    <span class="title">maybe_lexicon</span>
  </a>
</div>
<!-- apiref:nav:end -->
