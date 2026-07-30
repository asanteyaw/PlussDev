<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62bf86d0fb06 -->

# Catalog::nrecords

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::nrecords() const -> std::int64_t
```

## What It Does

Returns the row count shared by every stream in the catalog — but only when every stream
actually agrees on that count. Unlike a `DataFrame`, a `Catalog` doesn't enforce that all
its streams have the same length; `nrecords()` is the strict form that asserts they do, and
throws the moment it finds two streams of different lengths ("ragged").

```{important} Throws on a ragged catalog — use minrecords()/maxrecords()/tabelize() instead
If the catalog's streams don't all have the same length, `nrecords()` throws rather than
guessing which length is "the" record count. Use [`minrecords()`](minrecords.md) or
[`maxrecords()`](maxrecords.md) if you want a specific bound instead of an assertion, or
[`tabelize()`](tabelize.md) to align everything into a single `DataFrame` first. See
Example 2.
```

## Parameters

`None`

## Returns

`std::int64_t`
    The common row count, once every stream is confirmed to share it.

## Examples

Example 1. Every stream has the same length, so `nrecords()` returns that shared count.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

std::cout << ct.nrecords();
```

### Output

```text
3
```

Example 2. A ragged catalog — streams of different lengths — throws instead of guessing.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

try {
  auto n = ragged.nrecords();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::nrecords: catalog has ragged stream lengths; use minrecords()/maxrecords() or tabelize()
```

## Notes

- Throws on any length disagreement across streams — it never silently picks a "winning"
  length.
- A catalog with zero streams reports `nrecords() == 0` rather than throwing.
- See [`minrecords()`](minrecords.md)/[`maxrecords()`](maxrecords.md) for the ragged-safe
  alternatives, and [`shape()`](shape.md) (which uses `maxrecords()` internally) for a
  ragged-safe row/stream-count summary.

## See Also

- [`Catalog::minrecords`](minrecords.md) / [`Catalog::maxrecords`](maxrecords.md): the
  ragged-safe bounds this method's strict check is built from.
- [`Catalog::shape`](shape.md): a ragged-safe `(rows, streams)` summary.
- [`Catalog::tabelize`](tabelize.md): align a ragged catalog into a single `DataFrame`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="query.html">
    <span class="label">Next</span>
    <span class="title">query</span>
  </a>
</div>
