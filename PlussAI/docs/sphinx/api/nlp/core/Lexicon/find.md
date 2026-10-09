<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e26eb3fca908 -->

# `pluss::ai::nlp::Lexicon::find`

[Back to `pluss::ai::nlp::Lexicon`](../Lexicon.md)

## Signature

```cpp
const pluss::ai::nlp::Lexeme* pluss::ai::nlp::Lexicon::find(pluss::ai::nlp::StringId id) const noexcept;
const pluss::ai::nlp::Lexeme* pluss::ai::nlp::Lexicon::find(std::string_view value) const noexcept;
```

## What It Does

Returns the lexeme for an observed surface form, or `nullptr` when none exists. The overload taking a `StringId` looks up by the lexeme's own id, which is what a [`Token::orth_id`](../Token.md) carries. The overload taking text looks up by surface string. Neither creates a lexeme.

## Parameters

`StringId id`
: The lexeme's id, typically a token's `orth_id`.

`std::string_view value`
: The surface text to find, matched exactly, so `Laurel` and `laurel` are different.

## Returns

A `const Lexeme*`, or `nullptr` when the form was never observed.

## Errors

None. A miss returns `nullptr`.

## Practical Examples

### Example 1

Finding an observed form by its surface text.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  const nlp::Lexeme* found = lex.find("Laurel");
  std::cout << "found=" << (found != nullptr) << " text=" << found->text << "\n";
  return 0;
}
```

Output

```text
found=1 text=Laurel
```

### Example 2

Finding a form that was never observed returns `nullptr`.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  lex.observe("Laurel");
  std::cout << "missing_is_null=" << (lex.find("zebra") == nullptr) << "\n";
  return 0;
}
```

Output

```text
missing_is_null=1
```

### Example 3

Finding by id reaches the same lexeme a token's `orth_id` points to.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Lexicon lex{};
  const nlp::Lexeme& lexeme = lex.observe("Laurel");
  const nlp::Lexeme* by_id = lex.find(lexeme.id);
  std::cout << "by_id=" << by_id->text << "\n";
  return 0;
}
```

Output

```text
by_id=Laurel
```

## See Also

- [`observe`](observe.md) -- creates the lexeme these look up.
- [`lookup`](lookup.md) -- the id-only lookup.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear.html">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="find_morphology.html">
    <span class="label">Next</span>
    <span class="title">find_morphology</span>
  </a>
</div>
<!-- apiref:nav:end -->
