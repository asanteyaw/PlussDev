<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 992110e2ae69 -->

# Catalog::isnull

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::isnull() const -> Catalog
```

## What It Does

Reports per-cell validity across **every** stream — each stream's own `isnull()`, applied
independently. The result is a same-shaped catalog of `Bool`-dtype streams, `true`
wherever the source was `NULL`.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog, same keys and row count as `*this`, with every stream replaced by its
    own boolean `isnull()` result.

## Examples

Example 1. Check validity across a mixed numeric/text catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.isnull();
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
id                       | Series       | Series[bool]     | 3       | [false, true, false]                             
city                     | Series       | Series[bool]     | 3       | [false, false, true]                             
========================================================================================================================
```

Note that the *kind* of every resulting stream is `Series[bool]`, even though `city` was
originally `Note`-kind — validity results are always boolean series, regardless of the
source stream's own kind.

## Notes

- Every resulting stream is `Series[bool]`, regardless of the source stream's own payload
  kind.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::notnull`](notnull.md): the logical inverse.
- [`Catalog::invalid`](invalid.md): a related per-cell validity check.
- [`Catalog::dropnull`](dropnull.md) / [`Catalog::fillnull`](fillnull.md): act on `NULL`
  cells directly, rather than just reporting where they are.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="keyaxis.html">
    <span class="label">Next</span>
    <span class="title">keyaxis</span>
  </a>
</div>
