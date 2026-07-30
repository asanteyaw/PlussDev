<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c476aea47bd -->

# Catalog::maxrecords

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::maxrecords() const -> std::int64_t
```

## What It Does

Returns the length of the **longest** stream in the catalog — the ragged-safe counterpart
to [`nrecords()`](nrecords.md), which never throws regardless of whether the catalog's
streams agree on length. This is the value [`shape()`](shape.md) uses for its row-count
half, which is why `shape()` itself never throws on a ragged catalog.

## Parameters

`None`

## Returns

`std::int64_t`
    The length of the longest stream in the catalog, or `0` if the catalog has no streams.

## Examples

Example 1. All streams share the same length, so `maxrecords()` equals that length.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

std::cout << ct.maxrecords();
```

### Output

```text
3
```

Example 2. A ragged catalog — `maxrecords()` reports the longest stream's length, `3`,
without throwing.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

std::cout << ragged.maxrecords();
```

### Output

```text
3
```

Example 3. `shape()` uses `maxrecords()` for its row-count half — the two stay in sync
even on a ragged catalog.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

auto [rows, streams] = ragged.shape();
std::cout << ragged.maxrecords() << " == " << rows;
```

### Output

```text
3 == 3
```

## Notes

- Never throws on a ragged catalog — see [`nrecords()`](nrecords.md) for the strict form
  that does.
- An empty (zero-stream) catalog reports `0`, not an error.
- This is the value `shape()` reports as its row count.

## See Also

- [`Catalog::minrecords`](minrecords.md): the corresponding lower bound.
- [`Catalog::nrecords`](nrecords.md): the strict form that throws if streams disagree on
  length, rather than reporting a bound.
- [`Catalog::shape`](shape.md): a `(rows, streams)` summary built from this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
