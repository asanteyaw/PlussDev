<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8be9938b4b55 -->
# `pluss::ai::nlp::Doc::children_of`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
std::vector<std::int64_t> children_of(std::int64_t token_index) const;
```

## What It Does

Returns the token indices of every token whose `chief` (dependency
head) equals `token_index`, in token order -- the direct dependents of
one token in the dependency tree [`SyntaxStage`](../../tasks/SyntaxStage.md)
builds. Tokenization makes every token its own head (`chief` equals its own
index), so no token has a dependent until a syntax stage assigns real
heads; `children_of` therefore returns empty for a `Doc` that never went
through one.

## Parameters

`std::int64_t token_index`
: The token to query. Out-of-range values (negative, or past the last
  token) are treated the same as "no children" rather than throwing.

## Returns

A `std::vector<std::int64_t>`, possibly empty.

## Errors

None -- an invalid `token_index` returns an empty vector rather than
throwing.

## Practical Examples

### Example 1

The sentence root's direct dependents, after a full tag/parse
pipeline.

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
  std::cout << "root_children=" << doc.children_of(2).size() << "\n";
  return 0;
}
```

Output

```text
root_children=5
```

### Example 2

A leaf token -- one nothing else attaches to -- has no children.

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

  // token 5 is "NASA", a leaf of the parse.
  std::cout << "leaf_children=" << doc.children_of(5).size() << "\n";
  return 0;
}
```

Output

```text
leaf_children=0
```

### Example 3

Without a syntax stage, every token is its own head, so no token ever
has a dependent and `children_of` always returns empty.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Dr. Laurel writes APIs for NASA in New York.");

  std::cout << "children=" << doc.children_of(1).size() << "\n";
  return 0;
}
```

Output

```text
children=0
```

## See Also

- [`lineage_of`](lineage_of.md) -- walking upward instead of listing
  direct dependents.
- [`branch_of`](branch_of.md) -- the contiguous span a token's whole
  subtree covers.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="branch_of.html">
    <span class="label">Previous</span>
    <span class="title">branch_of</span>
  </a>
  <a class="next-link" href="ensure_lexicon.html">
    <span class="label">Next</span>
    <span class="title">ensure_lexicon</span>
  </a>
</div>
<!-- apiref:nav:end -->
