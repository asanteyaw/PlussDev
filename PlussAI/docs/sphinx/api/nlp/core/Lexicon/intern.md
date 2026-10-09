<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb8a4cb8c6ea -->

# `pluss::ai::nlp::Lexicon::intern`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
pluss::ai::nlp::StringId pluss::ai::nlp::Lexicon::intern(std::string_view value);
```

## What It Does

Adds `value` to the lexicon's text pool if it is new, and returns its stable id. Equal strings always return the same id. Interning does not create a lexeme, so `size()` is unchanged; only `texts()` grows. Use it to build a shared id space for strings a caller tracks by id, such as labels or keys.

## Parameters

`std::string_view value`
: The string to intern. An empty string is accepted and gets its own id.

## Returns

A `StringId`, a stable `std::uint64_t` key for `value`.

## Errors

None. Interning never fails.

## Practical Examples

### Example 1

Two equal strings intern to one id, and the lexicon stores one copy.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::StringId a = lex.intern("laurel");
  const nlp::StringId b = lex.intern("laurel");
  std::cout << "same_id=" << (a == b) << "\n";
  std::cout << "texts=" << lex.texts().size() << "\n";
  return 0;
}
```

Output

```text
same_id=1
texts=1
```

### Example 2

Interning does not create a lexeme, so the lexicon's own size stays at zero.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.intern("laurel");
  std::cout << "size=" << lex.size() << "\n";
  std::cout << "texts=" << lex.texts().size() << "\n";
  return 0;
}
```

Output

```text
size=0
texts=1
```

### Example 3

The returned id resolves back to the original text through `text`.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::StringId id = lex.intern("writes");
  std::cout << "text=" << lex.text(id) << "\n";
  return 0;
}
```

Output

```text
text=writes
```

## See Also

- [`lookup`](lookup.md) -- the read-only counterpart that never adds.
- [`text`](text.md) -- resolving an id back to its string.
- [`observe`](observe.md) -- the call that also creates a lexeme.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_morphology.html">
    <span class="label">Previous</span>
    <span class="title">find_morphology</span>
  </a>
  <a class="next-link" href="intern_morphology.html">
    <span class="label">Next</span>
    <span class="title">intern_morphology</span>
  </a>
</div>
<!-- apiref:nav:end -->
