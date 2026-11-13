<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c5a8c0565514 -->

# Slate::nt

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateNoteAccessor Slate::nt() const
```

## What It Does

Enters the dedicated text-operations accessor for this slate — string stripping,
casing, length, pattern matching, splitting, padding, encoding, and more, one method
per operation. `nt()` requires the slate to already be `Note`-kind.

Unlike [`tl()`](tl.md), which checks its payload kind **eagerly** (the moment you call
`.tl()`), `nt()` performs **no validation at all** when constructing the accessor —
it just binds a copy of the slate. The `Note`-kind requirement is only enforced
**lazily**, the first time you call one of the accessor's own methods (`strip()`,
`lower()`, `has()`, and so on each independently check the payload kind before doing
any work). This means `slate.nt()` alone never throws, even on a non-`Note` slate —
the throw only happens once you actually call a method on the accessor. See Example 2.

Unlike [`DataFrame::nt()`](../dataframe/nt.md), which operates across every selected
feature at once, this accessor is bound to a single slate — there's no equivalent of
`.features(...)` narrowing to do first.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference) and use
its left sidebar to jump straight to any method (`strip`, `has`, `count`, `split`,
`pad`, `replace`, and more).

## Parameters

`None`

## Returns

`SlateNoteAccessor`
    A lightweight accessor bound to this slate (its `Note`-kind isn't checked yet).
    Nearly every method on it returns a new `Slate` with the text operation applied;
    the source slate is never mutated.

## Examples

Example 1. Lowercase a `Note` slate, then check whether it starts with a prefix.

```cpp
using namespace pluss::table;

auto cities = Slate(pluss::make_note({"Accra", "TEMA", "Kumasi"}));

auto lowered = cities.nt().lower();
std::cout << lowered;
```

### Output

```text
0  accra
1  tema
2  kumasi

ptype: Note
```

```cpp
auto starts_a = cities.nt().starts("A");
std::cout << starts_a;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

`.lower()` returns a new `Note`-kind slate; `.starts(prefix)` returns a `bool`
`Series`-kind slate — only `"Accra"` (case-sensitively) starts with `"A"`.

Example 2. `nt()` itself never throws — the `Note`-kind check happens lazily, on the
accessor's first method call.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0}));
auto accessor = numbers.nt();  // does not throw

try {
  auto bad = accessor.lower();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::lower: expected Note payload
```

Compare this to [`.tl()`](tl.md) on the same non-matching slate, which throws
immediately at the `.tl()` call itself rather than waiting for a method call.

## Notes

- `nt()` performs no eager validation, unlike [`tl()`](tl.md) — a non-`Note` slate
  only fails once you call a method on the accessor, not at `.nt()` itself.
- Every accessor method independently enforces the `Note`-kind requirement with its
  own error message (for example `"Slate::lower: expected Note payload"`).
- No `options`/narrowing step is needed before calling `.nt()`, unlike
  [`DataFrame::nt()`](../dataframe/nt.md) — a `Slate` is already a single payload.

## See Also

- [`Slate::tl`](tl.md): the equivalent dedicated accessor for `Timeline`/`TimeIndex`
  payloads, which validates its payload kind eagerly instead of lazily.
- [`Slate::label`](label.md): the dedicated accessor for the case-label axis, which
  works regardless of the slate's own payload kind.
- [`DataFrame::nt`](../dataframe/nt.md): the frame-level equivalent, which requires
  every selected feature to already be `Note`-kind and validates eagerly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
