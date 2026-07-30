<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: afbf8e3665b6 -->

# Catalog::masked_select

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::masked_select(const pluss::Series& mask) const -> Catalog
```

## What It Does

A pure alias for [`filter()`](filter.md) — identical behavior, different name. See
[`filter()`](filter.md) for the full explanation, including the important callout about
per-stream length checking on a ragged catalog.

## Parameters

**mask** (`const pluss::Series&`)
    A `Bool`-dtype mask, `true` for rows to keep. Same requirements as
    [`filter()`](filter.md).

## Returns

`Catalog`
    A new catalog with every stream restricted to the rows where `mask` is `true` —
    identical to what `filter(mask)` would return.

## Examples

Example 1. Keep rows matching a boolean mask.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto mask = pluss::make_series({true, false, true, false, true}, pluss::codem::ScalarType::Bool);
auto out = ct.masked_select(mask);
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
id                       | Series       | Series[float64]  | 3       | [1, 3, 5]                                        
city                     | Note         | Note             | 3       | [Accra, Kumasi, Cape Coast]                      
========================================================================================================================
```

## Notes

- A pure alias for [`filter()`](filter.md) — no behavioral difference whatsoever.
- See `filter()`'s page for the full parameter/error-handling details, including the
  per-stream mask-length check.

## See Also

- [`Catalog::filter`](filter.md): the method this is an alias for — see that page for the
  full documentation.
- [`Catalog::where`](where.md): another alias for the same method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_txt.html">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
