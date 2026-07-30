<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ea5e08ef860c -->

# Catalog::tail

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::tail(std::int64_t n) const -> Catalog
```

## What It Does

Keeps the last `n` rows of **every** stream in the catalog — the mirror image of
[`head()`](head.md), delegating to each stream's own `tail(n)`. Like `head()`, it applies
independently per stream, so it works even on a ragged catalog.

## Parameters

**n** (`std::int64_t`)
    Number of trailing rows to keep from each stream.

## Returns

`Catalog`
    A new catalog with every stream trimmed to its last `n` rows.

## Examples

Example 1. Keep the last two rows of every stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.tail(2);
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
id                       | Series       | Series[float64]  | 2       | [4, 5]                                           
city                     | Note         | Note             | 2       | [Ho, Cape Coast]                                 
========================================================================================================================
```

## Notes

- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- Applies independently to each stream — works even on a ragged catalog.
- Requesting more rows than a stream has simply returns that stream unchanged.

## See Also

- [`Catalog::head`](head.md): the same idea, from the start of each stream instead of the
  end.
- [`Catalog::slice`](slice.md): an arbitrary `[start, stop)` row range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
