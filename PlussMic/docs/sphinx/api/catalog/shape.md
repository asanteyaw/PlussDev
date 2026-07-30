<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76c25b5d84d4 -->

# Catalog::shape

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::shape() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Returns a `(rows, streams)` pair summarizing the catalog's overall size in one call —
`{maxrecords(), nkeys()}`. Because it's built from [`maxrecords()`](maxrecords.md) rather
than [`nrecords()`](nrecords.md), `shape()` never throws on a ragged catalog (one whose
streams don't all share the same length); the row count it reports is the *longest*
stream's length in that case, not an assertion that all streams agree.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `{maxrecords(), nkeys()}` — the first element is the longest stream's length, the
    second is the number of streams.

## Examples

Example 1. A catalog whose streams all share the same length.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto [rows, streams] = ct.shape();
std::cout << "(" << rows << ", " << streams << ")";
```

### Output

```text
(3, 3)
```

Example 2. On a ragged catalog, `shape()` reports the longest stream's length as the row
count — it does not throw the way [`nrecords()`](nrecords.md) would.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

auto [rows, streams] = ragged.shape();
std::cout << "(" << rows << ", " << streams << ")";
```

### Output

```text
(3, 2)
```

## Notes

- Built from [`maxrecords()`](maxrecords.md), not `nrecords()` — never throws on a ragged
  catalog.
- The pair order is `(rows, streams)`, matching typical row-by-column shape conventions —
  not `(streams, rows)`.
- An explicitly empty catalog reports `(0, 0)`.

## See Also

- [`Catalog::nkeys`](nkeys.md) / [`Catalog::maxrecords`](maxrecords.md): the two values
  `shape()` combines.
- [`Catalog::nrecords`](nrecords.md): the strict, throwing row-count check this method
  deliberately avoids.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_.html">
    <span class="label">Previous</span>
    <span class="title">set_</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
