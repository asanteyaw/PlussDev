<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 83374ac40018 -->

# Catalog::cloc

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::cloc(std::int64_t row) const -> Catalog
Catalog::cloc(std::int64_t start, std::int64_t stop) const -> Catalog
```

## What It Does

Positional row access across every stream, in two shapes: a single row, or a `[start,
stop)` range. Both are thin wrappers over [`slice()`](slice.md) — `cloc(row)` is exactly
`slice(row, row + 1)`, and `cloc(start, stop)` is exactly `slice(start, stop)`, a pure
alias with no behavioral difference at all.

## Parameters

**row** (`std::int64_t`)
    A single row position to select. Produces a one-row catalog, not a scalar.

**start** (`std::int64_t`)
    Inclusive starting row position (two-argument overload).

**stop** (`std::int64_t`)
    Exclusive ending row position (two-argument overload).

## Returns

`Catalog`
    A new catalog restricted to the requested row or row range, across every stream.

## Examples

Example 1. Select a single row — the result is still a `Catalog`, just with one row in
each stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.cloc(2);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [3]                                              
city                     | Note         | Note             | 1       | [Kumasi]                                         
========================================================================================================================
```

Example 2. The two-argument overload behaves identically to [`slice()`](slice.md).

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.cloc(1, 3);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [2, 3]                                           
city                     | Note         | Note             | 2       | [Tema, Kumasi]                                   
========================================================================================================================
```

## Notes

- `cloc(row)` returns a one-row `Catalog`, not a scalar or a row-object — extract the
  value out of each stream afterward if you need it.
- `cloc(start, stop)` is a pure alias for [`slice(start, stop)`](slice.md) — no
  behavioral difference.

## See Also

- [`Catalog::slice`](slice.md): the method the two-argument overload is a direct alias for.
- [`Catalog::head`](head.md) / [`Catalog::tail`](tail.md): fixed-count row selection from
  either end.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="similarity.html">
    <span class="label">Next</span>
    <span class="title">similarity</span>
  </a>
</div>
