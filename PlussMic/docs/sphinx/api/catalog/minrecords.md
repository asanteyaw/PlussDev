<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 07846c0d126f -->

# Catalog::minrecords

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::minrecords() const -> std::int64_t
```

## What It Does

Returns the length of the **shortest** stream in the catalog — the ragged-safe counterpart
to [`nrecords()`](nrecords.md), which never throws regardless of whether the catalog's
streams agree on length. Useful as a conservative bound: the number of rows you could take
from every stream without running past the shortest one.

## Parameters

`None`

## Returns

`std::int64_t`
    The length of the shortest stream in the catalog, or `0` if the catalog has no streams.

## Examples

Example 1. All streams share the same length, so `minrecords()` equals that length.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

std::cout << ct.minrecords();
```

### Output

```text
3
```

Example 2. A ragged catalog — `minrecords()` reports the shortest stream's length, `2`,
without throwing (contrast with [`nrecords()`](nrecords.md), which would throw on this
same catalog).

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

std::cout << ragged.minrecords();
```

### Output

```text
2
```

Example 3. An explicitly empty catalog reports `minrecords() == 0`.

```cpp
pluss::kindle::Catalog ct(std::vector<std::pair<std::string, pluss::kindle::Stream>>{});
std::cout << ct.minrecords();
```

### Output

```text
0
```

## Notes

- Never throws on a ragged catalog — see [`nrecords()`](nrecords.md) for the strict form
  that does.
- An empty (zero-stream) catalog reports `0`, not an error.

## See Also

- [`Catalog::maxrecords`](maxrecords.md): the corresponding upper bound.
- [`Catalog::nrecords`](nrecords.md): the strict form that throws if streams disagree on
  length, rather than reporting a bound.
- [`Catalog::shape`](shape.md): a `(rows, streams)` summary built from `maxrecords()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
