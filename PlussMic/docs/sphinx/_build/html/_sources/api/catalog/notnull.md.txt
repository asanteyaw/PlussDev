<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a3214b3fdded -->

# Catalog::notnull

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::notnull() const -> Catalog
```

## What It Does

The logical inverse of [`isnull()`](isnull.md) — reports per-cell validity across every
stream, `true` wherever the source was **valid** (not `NULL`). Each stream's own
`notnull()`, applied independently.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog, same keys and row count as `*this`, with every stream replaced by its
    own boolean `notnull()` result.

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

auto out = ct.notnull();
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
id                       | Series       | Series[bool]     | 3       | [true, false, true]                              
city                     | Series       | Series[bool]     | 3       | [true, true, false]                              
========================================================================================================================
```

## Notes

- Every resulting stream is `Series[bool]`, regardless of the source stream's own payload
  kind.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- [`dropnull()`](dropnull.md) is conceptually this method's result ANDed across every
  stream, then used as a row filter.

## See Also

- [`Catalog::isnull`](isnull.md): the logical inverse.
- [`Catalog::dropnull`](dropnull.md): use validity to filter rows directly, rather than
  just reporting it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nkeys.html">
    <span class="label">Previous</span>
    <span class="title">nkeys</span>
  </a>
  <a class="next-link" href="nrows.html">
    <span class="label">Next</span>
    <span class="title">nrows</span>
  </a>
</div>
