<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91d6d8fabfd9 -->

# `pluss::ai::nlp::Lexicon::observe`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
[[nodiscard]] const pluss::ai::nlp::Lexeme& pluss::ai::nlp::Lexicon::observe(std::string_view value);
```

## What It Does

Records `value` as a surface form and returns its [`Lexeme`](../Lexeme.md). The first observation of a text creates the lexeme and interns its text, lowercase, prefix, suffix, and morphology. Later observations of the same text return the same lexeme object without changing the lexicon. Tokenization calls this for every token.

## Parameters

`std::string_view value`
: The surface text to observe.

## Returns

A `const Lexeme&` that stays valid until the lexicon is cleared.

## Errors

None.

## Practical Examples

### Example 1

Observing a word creates one lexeme with its lowercase form.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Hello");
  std::cout << "text=" << lexeme.text << " lower=" << lexeme.lower << "\n";
  std::cout << "size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
text=Hello lower=hello
size=1
```

### Example 2

Observing the same text again returns the same lexeme and leaves the size alone.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& first = lex.observe("Hello");
  const nlp::Lexeme& again = lex.observe("Hello");
  std::cout << "same_object=" << (&first == &again) << "\n";
  std::cout << "size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
same_object=1
size=1
```

### Example 3

Case matters: `Hello` and `hello` are separate lexemes that share a lowercase id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& upper = lex.observe("Hello");
  const nlp::Lexeme& lower = lex.observe("hello");
  std::cout << "same_id=" << (upper.id == lower.id) << "\n";
  std::cout << "same_lower_id=" << (upper.lower_id == lower.lower_id) << "\n";
  std::cout << "size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
same_id=0
same_lower_id=1
size=2
```

## See Also

- [`find`](find.md) -- returns a lexeme without creating one.
- [`Lexeme`](../Lexeme.md) -- the record returned.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="morphs.html">
    <span class="label">Previous</span>
    <span class="title">morphs</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
