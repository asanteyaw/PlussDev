<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 378799ee1f2c -->

# Catalog::head

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::head(std::int64_t n) const -> Catalog
```

## What It Does

Keeps the first `n` rows of **every** stream in the catalog, delegating directly to each
stream's own `head(n)`. Because it applies uniformly across every stream regardless of
their individual lengths, `head()` works even on a ragged catalog — each stream is
independently clipped to its own first `n` rows (or fewer, if a stream is already shorter
than `n`).

## Parameters

**n** (`std::int64_t`)
    Number of leading rows to keep from each stream.

## Returns

`Catalog`
    A new catalog with every stream trimmed to its first `n` rows.

## Examples

Example 1. Keep the first two rows of every stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.head(2);
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
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

## Notes

- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- Applies independently to each stream — works even on a ragged catalog, since it never
  compares stream lengths against each other.
- Requesting more rows than a stream has simply returns that stream unchanged (up to its
  own full length), rather than throwing or padding.

## See Also

- [`Catalog::tail`](tail.md): the same idea, from the end of each stream instead of the
  start.
- [`Catalog::slice`](slice.md): an arbitrary `[start, stop)` row range, rather than a fixed
  count from the beginning.
- [`Catalog::cloc`](cloc.md): a single-row or explicit-range selection, built on `slice()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="info.html">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>
