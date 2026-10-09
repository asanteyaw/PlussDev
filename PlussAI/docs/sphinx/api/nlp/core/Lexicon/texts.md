<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c7561107add6 -->

# `pluss::ai::nlp::Lexicon::texts`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
const pluss::ai::nlp::TextPool& pluss::ai::nlp::Lexicon::texts() const noexcept;
```

## What It Does

Returns the lexicon's [`TextPool`](../TextPool.md), read-only. Use it to inspect the interned strings, check their count, or look up ids without going through `intern`. Its size counts every string the lexicon has stored, including the lowercase, affix, and shape strings observing a word adds.

## Parameters

None.

## Returns

A `const TextPool&` owned by this lexicon.

## Errors

None.

## Practical Examples

### Example 1

The pool reports how many strings the lexicon has stored.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.intern("laurel");
  std::cout << "texts=" << lex.texts().size() << "\n";
  return 0;
}
```

Output

```text
texts=1
```

### Example 2

Observing one word stores several strings, all visible through the pool.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Hello");
  std::cout << "texts=" << lex.texts().size() << "\n";
  return 0;
}
```

Output

```text
texts=5
```

### Example 3

Looking up an id through the pool returns the same id as the lexicon's own lookup.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::StringId id = lex.intern("laurel");
  std::cout << "matches=" << (lex.texts().lookup("laurel") == id) << "\n";
  return 0;
}
```

Output

```text
matches=1
```

## See Also

- [`TextPool`](../TextPool.md) -- the pool's own page.
- [`intern`](intern.md) -- adds strings to it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="text.html">
    <span class="label">Previous</span>
    <span class="title">text</span>
  </a>
</div>
<!-- apiref:nav:end -->
