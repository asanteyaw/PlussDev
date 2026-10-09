<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 93626fc5ce30 -->

# `pluss::ai::nlp::Lexicon::size`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
std::size_t pluss::ai::nlp::Lexicon::size() const noexcept;
```

## What It Does

Returns how many distinct surface forms the lexicon holds lexemes for. It counts lexemes created by `observe` (and tokenization), not strings interned with `intern`. Use [`texts`](texts.md)`().size()` for the interned-string count.

## Parameters

None.

## Returns

A `std::size_t` lexeme count.

## Errors

None.

## Practical Examples

### Example 1

Observing distinct words increases the count; repeats do not.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  lex.observe("writes");
  lex.observe("Laurel");
  std::cout << "size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
size=2
```

### Example 2

Interning a string alone leaves the count at zero.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.intern("laurel");
  std::cout << "size=" << lex.size() << "\n";
  return 0;
}
```

Output

```text
size=0
```

### Example 3

A tokenized document's lexicon counts the lexemes its tokens produced.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs for NASA and Laurel emails help@pluss.dev with 42 accuracy.");
  std::cout << "size=" << doc.lexicon->size() << "\n";
  return 0;
}
```

Output

```text
size=12
```

## See Also

- [`texts`](texts.md) -- the interned-string count.
- [`clear`](clear.md) -- resets it to zero.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="observe.html">
    <span class="label">Previous</span>
    <span class="title">observe</span>
  </a>
  <a class="next-link" href="text.html">
    <span class="label">Next</span>
    <span class="title">text</span>
  </a>
</div>
<!-- apiref:nav:end -->
