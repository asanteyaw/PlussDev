<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8dbd4d075c2 -->

# `pluss::ai::nlp::Lexicon::intern_morphology`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
pluss::ai::nlp::StringId pluss::ai::nlp::Lexicon::intern_morphology(const pluss::ai::nlp::Morphology& value);
```

## What It Does

Stores a [`Morphology`](../Morphology.md) record in the lexicon's bank, keyed by its `spec` string, and returns its id. Records with equal `spec` share one id, and the first stored record is kept. Use it to add an analysis by hand; tokenization already stores each token's analysis automatically.

## Parameters

`const Morphology& value`
: The analysis to store. Its `spec` must be non-empty.

## Returns

The `StringId` of the feature class `value.spec` names.

## Errors

Throws when `value.spec` is empty.

## Practical Examples

### Example 1

Interning a record and reading it back by the returned id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  nlp::Morphology m{};
  m.shape = "Xxxxx";
  m.spec = "kind=word|case=title";
  const nlp::StringId id = lex.intern_morphology(m);
  std::cout << "shape=" << lex.find_morphology(id)->shape << "\n";
  return 0;
}
```

Output

```text
shape=Xxxxx
```

### Example 2

Records with the same `spec` share one id, so the second call stores nothing new.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  nlp::Morphology first{};
  first.spec = "kind=word|case=title";
  first.shape = "Xxxxx";
  nlp::Morphology second{};
  second.spec = "kind=word|case=title";
  second.shape = "Xxxxxxx";
  const nlp::StringId a = lex.intern_morphology(first);
  const nlp::StringId b = lex.intern_morphology(second);
  std::cout << "same_id=" << (a == b) << "\n";
  std::cout << "kept_shape=" << lex.find_morphology(a)->shape << "\n";
  return 0;
}
```

Output

```text
same_id=1
kept_shape=Xxxxx
```

### Example 3

An empty `spec` is rejected with an error rather than stored.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  bool threw = false;
  try {
    lex.intern_morphology(nlp::Morphology{});
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

## See Also

- [`find_morphology`](find_morphology.md) -- reads the record back.
- [`Morphology`](../Morphology.md) -- the record type.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intern.html">
    <span class="label">Previous</span>
    <span class="title">intern</span>
  </a>
  <a class="next-link" href="lookup.html">
    <span class="label">Next</span>
    <span class="title">lookup</span>
  </a>
</div>
<!-- apiref:nav:end -->
