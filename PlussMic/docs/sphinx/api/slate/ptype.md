<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7f0e0a23d2be -->

# Slate::ptype

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::ptype() const -> std::string
```

## What It Does

Returns the broad payload family as a display string: `"Series"`, `"Note"`,
`"Timeline"`, or `"TimeIndex"` — the same information [`kind()`](kind.md) returns as
a typed `SlateKind` enum, formatted for printing. This is the 1D analog of
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

auto numeric = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto text = Slate(pluss::make_note({"a", "b"}));
auto dates = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

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
  family (e.g. which numeric scalar type a `Series` slate holds).
- Calling `ptype()` on an undefined slate throws `"Slate: undefined handle"`.

## See Also

- [`Slate::kind`](kind.md): the same information as a typed `SlateKind` enum,
  for use in C++ comparisons and `switch` statements.
- [`Slate::dtype`](dtype.md): the concrete dtype, one level more specific.
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
