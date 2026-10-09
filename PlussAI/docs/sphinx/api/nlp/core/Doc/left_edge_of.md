<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9d8599b856ed -->
# `pluss::ai::nlp::Doc::left_edge_of`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::int64_t left_edge_of(std::int64_t token_index) const;
```

## What It Does

Returns `branch_of(token_index).token_start` -- the leftmost token
index in `token_index`'s whole dependency subtree. Equivalent to, but
cheaper to read than, computing [`branch_of`](branch_of.md) and reading
its `token_start` field directly when only the left boundary is
needed.

## Parameters

`std::int64_t token_index`
: The subtree's root token.

## Returns

A `std::int64_t` token index. `0` for an out-of-range `token_index`,
matching [`branch_of`](branch_of.md)'s own empty-span convention rather
than a negative sentinel.

## Errors

None.

## Practical Examples

### Example 1

A two-word phrase's left edge is the attached modifier, not the head
token itself.

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

  // token 8 is "York"; its subtree starts at "New" (token 7).
  std::cout << "left_edge=" << doc.left_edge_of(8) << "\n";
  return 0;
}
```

Output

```text
left_edge=7
```

### Example 2

A leaf token's left edge is itself.

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

  std::cout << "left_edge=" << doc.left_edge_of(5) << "\n";
  return 0;
}
```

Output

```text
left_edge=5
```

### Example 3

An out-of-range index returns `0`.

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

  std::cout << "left_edge=" << doc.left_edge_of(999) << "\n";
  return 0;
}
```

Output

```text
left_edge=0
```

## See Also

- [`right_edge_of`](right_edge_of.md) -- the matching right boundary.
- [`branch_of`](branch_of.md) -- the full span both edges come from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="entity_texts.html">
    <span class="label">Previous</span>
    <span class="title">entity_texts</span>
  </a>
  <a class="next-link" href="lemma_texts.html">
    <span class="label">Next</span>
    <span class="title">lemma_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
