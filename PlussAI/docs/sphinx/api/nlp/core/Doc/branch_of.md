<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3ae0aab318e -->
# `pluss::ai::nlp::Doc::branch_of`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
Span branch_of(std::int64_t token_index) const;
```

## What It Does

Returns the contiguous `Span` covering `token_index` and every token
reachable from it through [`children_of`](children_of.md), recursively
-- its whole dependency subtree, as a token range rather than a list of
indices. The returned `Span`'s `label` is always `"branch"`. Without a
syntax stage, a token's subtree is just itself, so `branch_of` returns
a single-token span.

## Parameters

`std::int64_t token_index`
: The subtree's root token. An out-of-range value returns an empty
  span (`token_start == token_end == 0`) rather than throwing.

## Returns

A [`Span`](../Span.md) whose `text(doc)` is the subtree's surface text.

## Errors

None -- an invalid `token_index` returns an empty span rather than
throwing.

## Practical Examples

### Example 1

A two-word proper noun's branch covers both tokens as one span.

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

  // token 8 is "York", with "New" (token 7) attached to it.
  const nlp::Span branch = doc.branch_of(8);
  std::cout << "text=" << branch.text(doc) << "\n";
  return 0;
}
```

Output

```text
text=New York
```

### Example 2

A leaf token's branch is just itself.

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

  // token 5 is "NASA", a leaf.
  std::cout << "text=" << doc.branch_of(5).text(doc) << "\n";
  return 0;
}
```

Output

```text
text=NASA
```

### Example 3

An out-of-range index returns an empty span, whose `text` is an empty
string.

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

  const nlp::Span branch = doc.branch_of(999);
  std::cout << "empty=" << branch.empty() << "\n";
  return 0;
}
```

Output

```text
empty=1
```

## See Also

- [`left_edge_of`](left_edge_of.md) / [`right_edge_of`](right_edge_of.md)
  -- just the two token-index boundaries of this same span.
- [`phrase_texts`](phrase_texts.md) -- the branches
  [`SyntaxStage`](../../tasks/SyntaxStage.md) itself groups into
  `Doc::phrases`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="children_of.html">
    <span class="label">Next</span>
    <span class="title">children_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
