<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 83ef20160849 -->

# `pluss::ai::nlp::Lexicon::text`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
const std::string& pluss::ai::nlp::Lexicon::text(pluss::ai::nlp::StringId id) const;
```

## What It Does

Returns the string stored under `id` in this lexicon's text pool. Pass ids this lexicon issued; an id from another lexicon names something unrelated here.

## Parameters

`StringId id`
: An id previously returned by `intern`, `lookup`, or a lexeme's id field.

## Returns

A `const std::string&` into the text pool.

## Errors

Behavior for an id this lexicon never issued is not guaranteed. Pass only ids obtained from this lexicon.

## Practical Examples

### Example 1

Resolving the id returned by `intern` gives back the original text.

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

### Example 2

A lexeme's own `id` resolves to its surface text.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Laurel");
  std::cout << "text=" << lex.text(lexeme.id) << "\n";
  return 0;
}
```

Output

```text
text=Laurel
```

### Example 3

Lowercased text is interned separately and resolves through its own id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Laurel");
  std::cout << "lower=" << lex.text(lexeme.lower_id) << "\n";
  return 0;
}
```

Output

```text
lower=laurel
```

## See Also

- [`intern`](intern.md) -- producing the ids this resolves.
- [`texts`](texts.md) -- the pool this reads from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="texts.html">
    <span class="label">Next</span>
    <span class="title">texts</span>
  </a>
</div>
<!-- apiref:nav:end -->
