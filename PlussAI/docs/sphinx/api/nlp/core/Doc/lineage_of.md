<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37e7dceb7ad0 -->
# `pluss::ai::nlp::Doc::lineage_of`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::int64_t> lineage_of(std::int64_t token_index) const;
```

## What It Does

Walks from `token_index` up through each token's own `chief` to the
sentence root, returning every index visited (starting with
`token_index` itself) in that order. The walk stops when it reaches a
token that is its own `chief`, a token whose `chief` is negative (the
unset default, or the root itself in some conventions), or a token it
has already visited (a cycle guard). Without a syntax stage, every token
is its own head, so the walk stops at the starting token and the result
is just `{token_index}`.

## Parameters

`std::int64_t token_index`
: The token to start from. An out-of-range value returns an empty
  vector rather than throwing.

## Returns

A `std::vector<std::int64_t>`, at least one element (`token_index`
itself) for any valid index.

## Errors

None -- an invalid `token_index` returns an empty vector rather than
throwing.

## Practical Examples

### Example 1

The path from a deeply-attached token up to the sentence root.

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

  // token 8 is "York", three steps below the root "writes" (idx 2).
  const std::vector<std::int64_t> lineage = doc.lineage_of(8);
  std::cout << "length=" << lineage.size() << "\n";
  for (const std::int64_t index : lineage) {
    std::cout << doc.tokens[static_cast<std::size_t>(index)].orth << " ";
  }
  std::cout << "\n";
  return 0;
}
```

Output

```text
length=3
York in writes 
```

### Example 2

The root's own lineage is just itself -- there is nothing above it.

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

  // token 2 is "writes", the sentence root.
  const std::vector<std::int64_t> lineage = doc.lineage_of(2);
  std::cout << "length=" << lineage.size() << " only=" << lineage.front() << "\n";
  return 0;
}
```

Output

```text
length=1 only=2
```

### Example 3

Without a syntax stage, every lineage is trivially just the starting
token.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs for NASA in New York.");

  std::cout << "length=" << doc.lineage_of(1).size() << "\n";
  return 0;
}
```

Output

```text
length=1
```

## See Also

- [`children_of`](children_of.md) -- walking downward instead of
  upward.
- [`branch_of`](branch_of.md) -- a token's whole subtree, rather than
  its path to the root.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lemma_texts.html">
    <span class="label">Previous</span>
    <span class="title">lemma_texts</span>
  </a>
  <a class="next-link" href="match_texts.html">
    <span class="label">Next</span>
    <span class="title">match_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
