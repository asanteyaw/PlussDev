# `pluss::ai::nlp::MorphBank`

The [`Morphology`](Morphology.md)-side counterpart to
[`TextPool`](TextPool.md): it stores one `Morphology` record per distinct
feature class and hands back a [`StringId`](Lexicon.md) for it, so every
token with the same analysis shares one record. A `Lexicon` owns one
`MorphBank`, attached to that lexicon's own `TextPool`.

Records are keyed by the morphology's `spec` string, not by the surface
form. Two different words with the same analysis (for example two
capitalized, alphabetic, five-letter words) therefore share one record and
one id. The first record stored for a given `spec` is kept; later values
with the same `spec` do not replace it.

## Signature

```cpp
pluss::ai::nlp::MorphBank::MorphBank(pluss::ai::nlp::TextPool* text_pool = nullptr) noexcept;
```

## Construction

A `MorphBank` is only usable once a [`TextPool`](TextPool.md) is attached,
either through the constructor or through `attach`.
Interning without an attached pool is an error. Inside a `Lexicon` this is
arranged automatically; a standalone bank needs the pool named explicitly.

## State

The stored records, keyed by their `StringId`, plus the attached pool that
gives each `spec` its id. `size()` reports how many distinct feature classes
the bank holds.

## Methods

### `attach(TextPool*)`

Points the bank at the pool that will assign ids to `spec` strings. Call it
before the first `intern`. Re-attaching to a different pool does not move
records already stored.

Returns `void`. Never throws.

### `intern(const Morphology&)`

Returns the id for `value`'s feature class, keyed by `value.spec`. The first
call with a given `spec` stores `value` and returns its id; later calls with
an equal `spec` return the same id and keep the original record. A `spec`
that is empty is rejected.

Returns a `StringId`. Throws when no pool is attached or when `spec` is
empty.

### `find(StringId) const`

Returns a pointer to the record stored under `id`, or `nullptr` when the id
names nothing in this bank.

Returns `const Morphology*`. Never throws.

### `size() const`

Returns how many distinct feature classes the bank holds.

Returns `std::size_t`. Never throws.

### `clear()`

Removes every record. Ids returned earlier name nothing afterward.

Returns `void`.

## Practical Examples

These examples build `Morphology` values by hand, setting `spec` explicitly,
because `intern` keys on it. Real tokens get a `spec` from tokenization.

### Example 1

Interning a record through an attached pool, then reading it back by id.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  nlp::MorphBank bank{&pool};

  nlp::Morphology title{};
  title.casing = nlp::CaseKind::Title;
  title.kind = nlp::TokenKind::Word;
  title.shape = "Xxxxx";
  title.spec = "kind=word|case=title";

  const nlp::StringId id = bank.intern(title);
  const nlp::Morphology* stored = bank.find(id);
  std::cout << "found=" << (stored != nullptr) << "\n";
  std::cout << "shape=" << stored->shape << "\n";
  return 0;
}
```

Output

```text
found=1
shape=Xxxxx
```

### Example 2

Two records with the same `spec` share one id, and the first stored record
is the one kept.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  nlp::MorphBank bank{&pool};

  nlp::Morphology first{};
  first.spec = "kind=word|case=title";
  first.shape = "Xxxxx";

  nlp::Morphology second{};
  second.spec = "kind=word|case=title";
  second.shape = "Xxxxxxx";

  const nlp::StringId a = bank.intern(first);
  const nlp::StringId b = bank.intern(second);
  std::cout << "same_id=" << (a == b) << "\n";
  std::cout << "size=" << bank.size() << "\n";
  std::cout << "kept_shape=" << bank.find(a)->shape << "\n";
  return 0;
}
```

Output

```text
same_id=1
size=1
kept_shape=Xxxxx
```

### Example 3

`find` reports `nullptr` for an id the bank never issued, and `clear`
empties the bank.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  nlp::MorphBank bank{&pool};

  nlp::Morphology upper{};
  upper.spec = "kind=word|case=upper";
  const nlp::StringId id = bank.intern(upper);

  std::cout << "missing_is_null=" << (bank.find(12345) == nullptr) << "\n";
  bank.clear();
  std::cout << "after_clear_size=" << bank.size() << "\n";
  std::cout << "after_clear_found=" << (bank.find(id) != nullptr) << "\n";
  return 0;
}
```

Output

```text
missing_is_null=1
after_clear_size=0
after_clear_found=0
```

## See Also

- [`TextPool`](TextPool.md) -- the pool a bank attaches to.
- [`Morphology`](Morphology.md) -- the record type this bank stores.
- [`Lexicon`](Lexicon.md) -- owns a bank and exposes it through
  `intern_morphology` and `find_morphology`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
