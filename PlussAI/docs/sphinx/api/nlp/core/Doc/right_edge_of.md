<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6a51259fd504 -->
# `pluss::ai::nlp::Doc::right_edge_of`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::int64_t right_edge_of(std::int64_t token_index) const;
```

## What It Does

Returns the rightmost token index in `token_index`'s whole dependency
subtree -- `branch_of(token_index).token_end - 1` for a non-empty
branch. Equivalent to, but cheaper to read than, computing
[`branch_of`](branch_of.md) and converting its exclusive `token_end`
to an inclusive last index directly.

## Parameters

`std::int64_t token_index`
: The subtree's root token.

## Returns

A `std::int64_t` token index. `0` for an out-of-range `token_index`,
matching [`left_edge_of`](left_edge_of.md)'s own convention.

## Errors

None.

## Practical Examples

### Example 1

A head token whose own index is already the right edge of its
subtree.

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

  // token 8 is "York", the head of the "New York" phrase; its subtree's
  // right edge is itself.
  std::cout << "right_edge=" << doc.right_edge_of(8) << "\n";
  return 0;
}
```

Output

```text
right_edge=8
```

### Example 2

A leaf token's right edge is itself.

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

  std::cout << "right_edge=" << doc.right_edge_of(5) << "\n";
  return 0;
}
```

Output

```text
right_edge=5
```

### Example 3

An out-of-range index returns `0`, the same sentinel
[`left_edge_of`](left_edge_of.md) uses.

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

  std::cout << "right_edge=" << doc.right_edge_of(999) << "\n";
  return 0;
}
```

Output

```text
right_edge=0
```

## See Also

- [`left_edge_of`](left_edge_of.md) -- the matching left boundary.
- [`branch_of`](branch_of.md) -- the full span both edges come from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="phrase_texts.html">
    <span class="label">Previous</span>
    <span class="title">phrase_texts</span>
  </a>
  <a class="next-link" href="sentence_texts.html">
    <span class="label">Next</span>
    <span class="title">sentence_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
