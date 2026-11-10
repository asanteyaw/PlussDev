<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7931eedf446d -->

# Catalog::distinct

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::distinct(DistinctOptions options = {}) const -> Catalog
```

### `DistinctOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |

## What It Does

Reduces **every** stream to its own distinct values, independently — each stream's own
`distinct()`, applied one at a time. This is not a row-level "unique combinations across
the whole catalog" operation the way a SQL `DISTINCT` over several columns would be; each
stream is de-duplicated on its own, with no relationship preserved between one stream's
distinct values and another's.

```{important} The result is very likely ragged — streams end up with different lengths
Because each stream is de-duplicated independently, two streams that started at the same
length will almost always end up at *different* lengths after `distinct()` — one might
have 3 distinct values, another 7. The resulting `Catalog` no longer has row-aligned
streams; don't treat row `i` of one stream as corresponding to row `i` of another after
calling this. See Example 1.
```

## Parameters

**options** (`DistinctOptions`, optional)
    `dropnull` (default `true`) excludes `NULL` from each stream's distinct-value list.

## Returns

`Catalog`
    A new catalog with every stream replaced by its own list of distinct values — streams
    are very likely to end up different lengths (ragged) as a result.

## Examples

Example 1. Two streams with different numbers of distinct values end up with different
lengths — `id` has 3 distinct values, `region` has 2.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "east"})),
});

auto out = ct.distinct();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, region]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
region                   | Note         | Note             | 2       | [east, west]                                     
========================================================================================================================
```

## Notes

- Each stream is de-duplicated independently — see the important callout above. The
  resulting catalog is very likely ragged.
- `dropnull` (default `true`) excludes `NULL` from each stream's own distinct-value list.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::ndistinct`](ndistinct.md): the distinct-value *count* per stream, rather than
  the values themselves.
- [`Catalog::freq`](freq.md): distinct values paired with how often each occurred.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="display.html">
    <span class="label">Previous</span>
    <span class="title">display</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
