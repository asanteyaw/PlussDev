# `pluss::ai::nlp::TextPool`

A deterministic string-interning table. Every distinct string a
[`Lexicon`](Lexicon.md) sees is stored once, and is addressed afterward by
a stable [`StringId`](Lexicon.md) -- a plain `std::uint64_t`. Two equal
strings always receive the same id, so comparing ids is a cheap stand-in
for comparing strings.

A `TextPool` is owned by its `Lexicon` in normal use; callers working
through a [`Doc`](Doc.md) reach it as `doc.lexicon->texts()`. A standalone
pool is useful when a caller wants the same interning behavior outside a
document.

## Signature

```cpp
pluss::ai::nlp::TextPool::TextPool() = default;
```

## Construction

A default-constructed pool is empty. Interning is the only way strings
enter it, so a pool never holds a string nobody asked for.

## State

Only the interned strings and their ids. `size()` reports how many
distinct strings are stored; ids are assigned in first-seen order but
callers should treat them as opaque, stable keys rather than as indices.

## Methods

### `intern(std::string_view)`

Returns the id for `value`, storing the string first if it is new. Calling
it again with an equal string returns the same id and stores nothing new.
This is the call a `Lexicon` makes for every surface form it observes.

Returns a `StringId`. Never returns `0` for a stored string.

### `lookup(std::string_view) const`

Returns the existing id for `value`, or `0` when the string has never been
interned. Unlike `intern`, it never adds anything, so it is the right call
for a read-only check.

Returns a `StringId`, `0` on a miss.

### `contains(StringId) const`

Reports whether `id` names a string currently stored in the pool.

Returns `bool`. Never throws.

### `at(StringId) const`

Returns the string stored under `id`. Pass only ids this pool actually
returned; an id from another pool, or one cleared away, names nothing
here.

Returns `const std::string&`.

### `size() const`

Returns how many distinct strings the pool holds.

Returns `std::size_t`. Never throws.

### `clear()`

Removes every stored string. Ids handed out earlier become invalid for
this pool, so a caller must not keep them across a `clear`.

Returns `void`.

## Practical Examples

### Example 1

Interning two equal strings returns one id, and the pool stores only one
copy.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  const nlp::StringId first = pool.intern("hello");
  const nlp::StringId second = pool.intern("hello");

  std::cout << "same_id=" << (first == second) << "\n";
  std::cout << "size=" << pool.size() << "\n";
  std::cout << "text=" << pool.at(first) << "\n";
  return 0;
}
```

Output

```text
same_id=1
size=1
text=hello
```

### Example 2

`lookup` is the read-only probe: it finds an interned string and reports
`0` for one that was never added.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  const nlp::StringId stored = pool.intern("laurel");

  std::cout << "found=" << (pool.lookup("laurel") == stored) << "\n";
  std::cout << "missing=" << pool.lookup("zebra") << "\n";
  std::cout << "size=" << pool.size() << "\n";
  return 0;
}
```

Output

```text
found=1
missing=0
size=1
```

### Example 3

`clear` empties the pool, and `contains` then reports a previously valid
id as gone.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::TextPool pool{};
  const nlp::StringId id = pool.intern("api");
  std::cout << "before=" << pool.contains(id) << "\n";

  pool.clear();
  std::cout << "after=" << pool.contains(id) << "\n";
  std::cout << "size=" << pool.size() << "\n";
  return 0;
}
```

Output

```text
before=1
after=0
size=0
```

## See Also

- [`Lexicon`](Lexicon.md) -- owns a `TextPool` and interns every surface
  form through it.
- [`MorphBank`](MorphBank.md) -- the morphology-side table attached to
  the same pool.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
