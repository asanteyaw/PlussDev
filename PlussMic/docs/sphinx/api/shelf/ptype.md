<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7f0e0a23d2be -->

# Shelf::ptype

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
std::string Shelf::ptype() const
```

## What It Does

Returns the broad payload family as a display string: `"Series"`, `"Note"`,
`"Timeline"`, or `"TimeIndex"` — the same information [`kind()`](kind.md) returns as
a typed `ShelfKind`, formatted for printing. This is the 1D analog of
`DataFrame::ptypes()`, which reports the same family names per feature.

## Parameters

`None`

## Returns

`std::string`
    One of `"Series"`, `"Note"`, `"Timeline"`, `"TimeIndex"`.

## Examples

Example 1. `ptype()` across all four payload kinds.

```cpp
using namespace pluss::table;

auto numeric = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto text = Shelf(pluss::make_note({"a", "b"}));
auto dates = Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << numeric.ptype() << "\n";
std::cout << text.ptype() << "\n";
std::cout << dates.ptype();
```

### Output

```text
Series
Note
Timeline
```

## Notes

- `ptype()` is for dispatching or displaying by payload *family*. Use
  [`dtype()`](dtype.md) when you need the concrete storage detail within that
  family (e.g. which numeric scalar type a `Series` shelf holds).
- Calling `ptype()` on an undefined shelf throws `"Shelf: undefined handle"`.

## See Also

- [`Shelf::kind`](kind.md): the same information as a typed `ShelfKind`,
  for use in C++ comparisons and `switch` statements.
- [`Shelf::dtype`](dtype.md): the concrete dtype, one level more specific.
- [`DataFrame::ptypes`](../dataframe/ptypes.md): the same summary across every
  feature of an entire frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="put.html">
    <span class="label">Next</span>
    <span class="title">put</span>
  </a>
</div>
