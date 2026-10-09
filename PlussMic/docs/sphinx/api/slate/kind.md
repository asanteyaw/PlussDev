<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63b53be03e05 -->

# Slate::kind

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::kind() const -> SlateKind
```

## What It Does

Returns the `SlateKind` enum value identifying which payload family a slate
currently stores: `Series` (numeric/bool), `Note` (text), `Timeline`, or
`TimeIndex`. This is the typed enum form; [`ptype()`](ptype.md) returns the same
information as a human-readable string (`"Series"`, `"Note"`, `"Timeline"`,
`"TimeIndex"`) — use `kind()` for `switch`/comparison logic in C++ and `ptype()` when
you just want to display or print the family name.

Every payload-kind-specific method (the string family, the temporal accessors, and
so on) checks `kind()` internally and throws when it doesn't match — this is the
same check surfaced as, for example, `"Slate::upper: expected Note payload"`.

## Parameters

`None`

## Returns

`SlateKind`
    One of `SlateKind::Series`, `SlateKind::Note`, `SlateKind::Timeline`,
    `SlateKind::TimeIndex`.

## Examples

Example 1. `kind()` reflects the constructor used, regardless of the actual values
stored.

```cpp
using namespace pluss::table;

auto numeric = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto text = Slate(pluss::make_note({"a", "b"}));
auto dates = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << (numeric.kind() == SlateKind::Series) << "\n";
std::cout << (text.kind() == SlateKind::Note) << "\n";
std::cout << (dates.kind() == SlateKind::Timeline);
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
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::upper: expected Note payload
```

## Notes

- Calling `kind()` on an undefined slate throws `"Slate: undefined handle"`.
- Every method restricted to one payload family checks `kind()` first and throws a
  method-specific message when it doesn't match.

## See Also

- [`Slate::ptype`](ptype.md): the same information as a display string.
- [`Slate::dtype`](dtype.md): the more specific concrete storage dtype within a
  kind (e.g. which `Series` scalar type).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kurtosis.html">
    <span class="label">Next</span>
    <span class="title">kurtosis</span>
  </a>
</div>
