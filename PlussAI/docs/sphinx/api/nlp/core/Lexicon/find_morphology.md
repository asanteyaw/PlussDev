<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4801cb6b585f -->

# `pluss::ai::nlp::Lexicon::find_morphology`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
const pluss::ai::nlp::Morphology* pluss::ai::nlp::Lexicon::find_morphology(pluss::ai::nlp::StringId id) const noexcept;
```

## What It Does

Returns the stored [`Morphology`](../Morphology.md) for an id from [`intern_morphology`](intern_morphology.md), or from a token's or lexeme's `morphology_id`. Returns `nullptr` when the id names nothing in this lexicon's bank.

## Parameters

`StringId id`
: The morphology id to read.

## Returns

A `const Morphology*`, or `nullptr` for an id this bank does not hold.

## Errors

None. An unknown id returns `nullptr`.

## Practical Examples

### Example 1

Reading a record back by the id `intern_morphology` returned.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  nlp::Morphology m{};
  m.spec = "kind=word|case=upper";
  m.shape = "XXXX";
  const nlp::StringId id = lex.intern_morphology(m);
  std::cout << "shape=" << lex.find_morphology(id)->shape << "\n";
  return 0;
}
```

Output

```text
shape=XXXX
```

### Example 2

A lexeme's `morphology_id` reads back the same record as its `morphology` field.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Laurel");
  const nlp::Morphology* stored = lex.find_morphology(lexeme.morphology_id);
  std::cout << "casing_matches=" << (stored->casing == lexeme.morphology.casing) << "\n";
  return 0;
}
```

Output

```text
casing_matches=1
```

### Example 3

An id the bank never issued returns `nullptr`.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  std::cout << "unknown_is_null=" << (lex.find_morphology(7) == nullptr) << "\n";
  return 0;
}
```

Output

```text
unknown_is_null=1
```

## See Also

- [`intern_morphology`](intern_morphology.md) -- stores the record.
- [`morphs`](morphs.md) -- the bank these read from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find.html">
    <span class="label">Previous</span>
    <span class="title">find</span>
  </a>
  <a class="next-link" href="intern.html">
    <span class="label">Next</span>
    <span class="title">intern</span>
  </a>
</div>
<!-- apiref:nav:end -->
