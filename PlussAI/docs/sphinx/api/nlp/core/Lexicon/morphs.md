<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 755ca9cc20f3 -->

# `pluss::ai::nlp::Lexicon::morphs`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
const pluss::ai::nlp::MorphBank& pluss::ai::nlp::Lexicon::morphs() const noexcept;
```

## What It Does

Returns the lexicon's [`MorphBank`](../MorphBank.md), read-only. Its size counts distinct feature classes, which is usually much smaller than the lexeme count, since many words share one analysis.

## Parameters

None.

## Returns

A `const MorphBank&` owned by this lexicon.

## Errors

None.

## Practical Examples

### Example 1

Each observed word stores a morphology record, keyed by its feature class.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  std::cout << "morphs=" << lex.morphs().size() << "\n";
  return 0;
}
```

Output

```text
morphs=1
```

### Example 2

Words with the same analysis share one bank record.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  lex.observe("Hello");
  std::cout << "lexemes=" << lex.size() << " morphs=" << lex.morphs().size() << "\n";
  return 0;
}
```

Output

```text
lexemes=2 morphs=1
```

### Example 3

Looking up a stored record through the bank returns it.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Laurel");
  std::cout << "found=" << (lex.morphs().find(lexeme.morphology_id) != nullptr) << "\n";
  return 0;
}
```

Output

```text
found=1
```

## See Also

- [`MorphBank`](../MorphBank.md) -- the bank's own page.
- [`find_morphology`](find_morphology.md) -- reads a record by id.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lookup.html">
    <span class="label">Previous</span>
    <span class="title">lookup</span>
  </a>
  <a class="next-link" href="observe.html">
    <span class="label">Next</span>
    <span class="title">observe</span>
  </a>
</div>
<!-- apiref:nav:end -->
