<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1452420d0fec -->

# Slate::dtype

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::dtype() const -> std::string
```

## What It Does

Returns the concrete dtype name of the underlying payload — one level more specific
than [`ptype()`](ptype.md)'s broad family name. For a `Series`-kind slate this is
the scalar type's own name (`"float64"`, `"bool"`, `"int64"`, and so on); for `Note`
it's `"utf8"` (or `"bytes"`, for byte-string payloads); for `Timeline`/`TimeIndex`
it's `"datetime64[ns, <tz>]"`, including the timezone.

## Parameters

`None`

## Returns

`std::string`
    The concrete dtype name, formatted per payload kind as described above.

## Examples

Example 1. `dtype()` across the payload kinds — note how it differs from
[`ptype()`](ptype.md), which would report `"Series"` / `"Note"` / `"Timeline"` for
the same slates.

```cpp
using namespace pluss::table;

auto numeric = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto text = Slate(pluss::make_note({"a", "b"}));
auto dates = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << numeric.dtype() << "\n";
std::cout << text.dtype() << "\n";
std::cout << dates.dtype();
```

### Output

```text
float64
utf8
datetime64[ns, UTC]
```

`dates` was built from a UTC-suffixed (`Z`) timestamp string, so its `Timeline`
carries the `UTC` timezone — `dtype()` includes it directly in the string, unlike
`ptype()`, which would just say `"Timeline"` regardless of timezone.

## Notes

- `Series`-kind slates report their scalar type by name (`"float64"`, `"bool"`,
  `"int64"`, etc.) — the same names `DataFrame::dtypes()` reports per feature.
- `Note`-kind slates report `"utf8"` (or `"bytes"` for a byte-string payload).
- `Timeline`/`TimeIndex`-kind slates report `"datetime64[ns, <tz>]"`, including the
  timezone.
- Calling `dtype()` on an undefined slate throws `"Slate: undefined handle"`.

## See Also

- [`Slate::ptype`](ptype.md): the broader payload family, without concrete storage
  detail.
- [`DataFrame::dtypes`](../dataframe/dtypes.md): the same concrete-dtype summary
  across every feature of an entire frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
