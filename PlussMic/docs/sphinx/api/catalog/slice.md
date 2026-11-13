<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8dac3990c8d4 -->

# Catalog::slice

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Keeps rows `[start, stop)` — inclusive start, exclusive stop — of **every** stream in the
catalog, delegating to each stream's own `slice()`. [`cloc(start, stop)`](cloc.md) is a
pure alias for this same method; [`cloc(row)`](cloc.md) is built on it too, as
`slice(row, row + 1)`.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting row position.

**stop** (`std::int64_t`)
    Exclusive ending row position.

## Returns

`Catalog`
    A new catalog with every stream restricted to rows `[start, stop)`.

## Examples

Example 1. Slice out a contiguous middle range of rows from every stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.slice(1, 3);
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

- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- Applies independently to each stream — works even on a ragged catalog.
- [`cloc(start, stop)`](cloc.md) is a pure alias for this method; `cloc()`'s one-argument
  overload builds on it as `slice(row, row + 1)`.

## See Also

- [`Catalog::cloc`](cloc.md): the alias, plus a single-row shorthand built on this method.
- [`Catalog::head`](head.md) / [`Catalog::tail`](tail.md): fixed-count slices anchored to
  the start or end, rather than an arbitrary range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
