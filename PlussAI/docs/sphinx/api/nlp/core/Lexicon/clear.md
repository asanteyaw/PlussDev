<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: da7246cfde18 -->

# `pluss::ai::nlp::Lexicon::clear`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
void pluss::ai::nlp::Lexicon::clear();
```

## What It Does

Empties the lexicon: every interned string, every morphology record, and every lexeme. Ids and references obtained earlier become invalid, so do not keep them across a `clear`.

## Parameters

None.

## Returns

Nothing (`void`).

## Errors

None.

## Practical Examples

### Example 1

After a clear, the lexicon's size and both stores are empty.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  lex.clear();
  std::cout << "size=" << lex.size() << " texts=" << lex.texts().size() << " morphs=" << lex.morphs().size() << "\n";
  return 0;
}
```

Output

```text
size=0 texts=0 morphs=0
```

### Example 2

A cleared lexicon can be reused, and it assigns fresh ids to new text.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  lex.clear();
  const nlp::Lexeme& again = lex.observe("writes");
  std::cout << "text=" << again.text << " size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
text=writes size=1
```

### Example 3

Lookups after a clear find nothing that was stored before.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  lex.clear();
  std::cout << "found=" << (lex.find("Laurel") != nullptr) << "\n";
  return 0;
}
```

Output

```text
found=0
```

## See Also

- [`size`](size.md) -- reads zero after a clear.
- [`Lexicon`](../Lexicon.md) -- the lexicon's own page.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="find.html">
    <span class="label">Next</span>
    <span class="title">find</span>
  </a>
</div>
<!-- apiref:nav:end -->
