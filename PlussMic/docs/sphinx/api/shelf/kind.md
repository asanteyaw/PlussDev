<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63b53be03e05 -->

# Shelf::kind

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfKind Shelf::kind() const
```

## What It Does

Returns the `ShelfKind` enum value identifying which payload family a shelf
currently stores: `Series` (numeric/bool), `Note` (text), `Timeline`, or
`TimeIndex`. This is the typed enum form; [`ptype()`](ptype.md) returns the same
information as a human-readable string (`"Series"`, `"Note"`, `"Timeline"`,
`"TimeIndex"`) — use `kind()` for `switch`/comparison logic in C++ and `ptype()` when
you just want to display or print the family name.

Every payload-kind-specific method (the string family, the temporal accessors, and
so on) checks `kind()` internally and throws when it doesn't match — this is the
same check surfaced as, for example, `"Shelf::upper: expected Note payload"`.

## Parameters

`None`

## Returns

`ShelfKind`
    One of `ShelfKind::Series`, `ShelfKind::Note`, `ShelfKind::Timeline`,
    `ShelfKind::TimeIndex`.

## Examples

Example 1. `kind()` reflects the constructor used, regardless of the actual values
stored.

```cpp
using namespace pluss::table;

auto numeric = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto text = Shelf(pluss::make_note({"a", "b"}));
auto dates = Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << (numeric.kind() == ShelfKind::Series) << "\n";
std::cout << (text.kind() == ShelfKind::Note) << "\n";
std::cout << (dates.kind() == ShelfKind::Timeline);
```

### Output

```text
1
1
1
```

Example 2. `kind()` drives the exact "wrong payload" throw every kind-specific
method raises.

```cpp
try {
  auto bad = numeric.upper();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::upper: expected Note payload
```

## Notes

- Calling `kind()` on an undefined shelf throws `"Shelf: undefined handle"`.
- Every method restricted to one payload family checks `kind()` first and throws a
  method-specific message when it doesn't match.

## See Also

- [`Shelf::ptype`](ptype.md): the same information as a display string.
- [`Shelf::dtype`](dtype.md): the more specific concrete storage dtype within a
  kind (e.g. which `Series` scalar type).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isposinf.html">
    <span class="label">Previous</span>
    <span class="title">isposinf</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
