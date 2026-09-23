<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 738f9c09c40f -->

# Catalog::nkeys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::nkeys() const -> std::int64_t
```

## What It Does

Returns the number of named streams (columns) in the catalog — the "width" of the catalog,
not its row count. [`size()`](size.md) is a pure alias for this same value; use whichever
name reads better at the call site. Requires a defined catalog; throws otherwise.

```{important} This counts streams, not rows
`nkeys()` (and its alias `size()`) count the number of named streams the catalog holds —
they say nothing about how many rows each stream has. For row counts, see
[`nrows()`](nrows.md)/[`minrows()`](minrows.md)/[`maxrows()`](maxrows.md),
or the row half of [`shape()`](shape.md).
```

## Parameters

`None`

## Returns

`std::int64_t`
    The number of named streams in the catalog.

## Examples

Example 1. A three-stream catalog reports `nkeys() == 3`, regardless of how many rows each
stream holds.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << ct.nkeys();
```

### Output

```text
3
```

Example 2. An explicitly empty catalog (defined, but with no streams) reports
`nkeys() == 0`.

```cpp
pluss::kindle::Catalog ct(std::vector<std::pair<std::string, pluss::kindle::Stream>>{});
std::cout << ct.nkeys();
```

### Output

```text
0
```

Example 3. Calling `nkeys()` on an undefined catalog throws.

```cpp
pluss::kindle::Catalog ct;
try {
  auto n = ct.nkeys();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog: undefined handle
```

## Notes

- Counts streams, not rows — see the important callout above.
- [`size()`](size.md) is a pure alias for `nkeys()`.
- Throws `"Catalog: undefined handle"` on an undefined catalog — see
  [`defined()`](defined.md).

## See Also

- [`Catalog::size`](size.md): a pure alias for this method.
- [`Catalog::nrows`](nrows.md) / [`Catalog::minrows`](minrows.md) /
  [`Catalog::maxrows`](maxrows.md): the row-count counterparts.
- [`Catalog::shape`](shape.md): both dimensions (rows, streams) in one call.
- [`Catalog::empty`](empty.md): shorthand for `nkeys() == 0`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
