<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 426d93177797 -->
# `pluss::ai::nlp::Doc::maybe_lexicon`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
const Lexicon* maybe_lexicon() const noexcept;
```

## What It Does

Returns `lexicon.get()` -- a read-only pointer, `nullptr` only if a
caller has explicitly cleared `lexicon` by hand (a default-constructed
`Doc` always has a non-null one). Use this over
[`ensure_lexicon`](ensure_lexicon.md) when a caller wants to inspect the
`Lexicon` without risking an allocation as a side effect, or needs a
`const Doc`, since `ensure_lexicon` is non-`const`.

## Parameters

None.

## Returns

A `const Lexicon*`, or `nullptr`.

## Errors

None.

## Practical Examples

### Example 1

A default-constructed `Doc`'s `lexicon` is non-null from the start.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::Doc doc{};
  std::cout << "non_null_by_default=" << (doc.maybe_lexicon() != nullptr) << "\n";
  return 0;
}
```

Output

```text
non_null_by_default=1
```

### Example 2

Reading the lexicon's size on a `const Doc`, after a `Workflow` has
tokenized it.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");
  std::cout << "size=" << doc.maybe_lexicon()->size() << "\n";
  return 0;
}
```

Output

```text
size=4
```

### Example 3

After explicitly clearing `lexicon`, `maybe_lexicon` reports `nullptr`
-- unlike [`ensure_lexicon`](ensure_lexicon.md), it never recreates one.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Doc doc{};
  doc.lexicon = nullptr;
  std::cout << "null=" << (doc.maybe_lexicon() == nullptr) << "\n";
  return 0;
}
```

Output

```text
null=1
```

## See Also

- [`ensure_lexicon`](ensure_lexicon.md) -- the allocating, non-`const`
  counterpart.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match_texts.html">
    <span class="label">Previous</span>
    <span class="title">match_texts</span>
  </a>
  <a class="next-link" href="phrase_texts.html">
    <span class="label">Next</span>
    <span class="title">phrase_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
