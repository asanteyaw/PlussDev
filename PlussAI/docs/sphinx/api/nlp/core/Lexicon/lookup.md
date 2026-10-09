<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dce623934414 -->

# `pluss::ai::nlp::Lexicon::lookup`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
pluss::ai::nlp::StringId pluss::ai::nlp::Lexicon::lookup(std::string_view value) const;
```

## What It Does

Returns the id of `value` if it has been interned, or `0` if it has not. Unlike `intern`, it never adds anything, so it is the right call for a read-only membership check on a lexicon a caller does not want to modify.

## Parameters

`std::string_view value`
: The string to look up.

## Returns

The existing `StringId`, or `0` when `value` has never been interned.

## Errors

None. A miss is reported as `0`, not as an error.

## Practical Examples

### Example 1

A string that was interned is found by its id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::StringId stored = lex.intern("laurel");
  std::cout << "found=" << (lex.lookup("laurel") == stored) << "\n";
  return 0;
}
```

Output

```text
found=1
```

### Example 2

A string never interned returns `0`, and the lexicon is not changed by the lookup.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.intern("laurel");
  std::cout << "missing=" << lex.lookup("zebra") << "\n";
  std::cout << "texts=" << lex.texts().size() << "\n";
  return 0;
}
```

Output

```text
missing=0
texts=1
```

### Example 3

Looking up a surface form observed by a lexicon reports the id its text received.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  std::cout << "observed=" << (lex.lookup("Laurel") != 0) << "\n";
  return 0;
}
```

Output

```text
observed=1
```

## See Also

- [`intern`](intern.md) -- adds the string when it is missing.
- [`find`](find.md) -- the lexeme-returning lookup.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intern_morphology.html">
    <span class="label">Previous</span>
    <span class="title">intern_morphology</span>
  </a>
  <a class="next-link" href="morphs.html">
    <span class="label">Next</span>
    <span class="title">morphs</span>
  </a>
</div>
<!-- apiref:nav:end -->
