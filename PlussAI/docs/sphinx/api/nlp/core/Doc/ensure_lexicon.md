<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 17bb17cbd788 -->
# `pluss::ai::nlp::Doc::ensure_lexicon`

[Back to `pluss::ai::nlp::Doc`](../Doc.md)

## Signature

```cpp
Lexicon& ensure_lexicon();
```

## What It Does

Returns a reference to `lexicon`, lazily constructing a fresh, empty
[`Lexicon`](../Lexicon.md) and assigning it first if `lexicon` is
currently null. In practice `lexicon` is almost never null -- `Doc`'s
own default member initializer already points it at a fresh `Lexicon`
at construction time -- so this is usually just a convenient
non-`const` reference to the existing one; the lazy-recreation path only
matters after a caller has explicitly cleared the field by hand (setting
`doc.lexicon = nullptr`).

## Parameters

None.

## Returns

A `Lexicon&` into `*lexicon`, never null after the call returns.

## Errors

None.

## Practical Examples

### Example 1

A freshly default-constructed `Doc` already has an empty `Lexicon` --
`ensure_lexicon` does not need to create anything here.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Doc doc{};
  std::cout << "size=" << doc.ensure_lexicon().size() << "\n";
  return 0;
}
```

Output

```text
size=0
```

### Example 2

After a `Workflow` has tokenized some text, `ensure_lexicon` returns
the same `Lexicon` that tokenization already populated.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc doc = wf.infer("Laurel writes APIs.");
  std::cout << "size=" << doc.ensure_lexicon().size() << "\n";
  return 0;
}
```

Output

```text
size=4
```

### Example 3

The lazy-recreation path: after explicitly clearing `lexicon` to
`nullptr`, `ensure_lexicon` builds a fresh, empty one in its place.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Doc doc{};
  doc.lexicon = nullptr;
  std::cout << "null_before=" << (doc.maybe_lexicon() == nullptr) << "\n";
  std::cout << "size_after=" << doc.ensure_lexicon().size() << "\n";
  return 0;
}
```

Output

```text
null_before=1
size_after=0
```

## See Also

- [`maybe_lexicon`](maybe_lexicon.md) -- the read-only, never-allocating
  counterpart.
- [`Lexicon`](../Lexicon.md) -- what this returns a reference to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="children_of.html">
    <span class="label">Previous</span>
    <span class="title">children_of</span>
  </a>
  <a class="next-link" href="entity_texts.html">
    <span class="label">Next</span>
    <span class="title">entity_texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
