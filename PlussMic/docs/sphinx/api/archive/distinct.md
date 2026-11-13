<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b46761567d97 -->

# Archive::distinct

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::distinct(DistinctOptions options = {}) const
```

### `DistinctOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |

## What It Does

Returns a new archive where every catalog is replaced by its own
[`Catalog::distinct()`](../catalog/distinct.md) result — each stream reduced to its
distinct values independently, per catalog. `options.dropnull` (default `true`) controls
whether a `NULL` counts as one of the distinct values.

## Parameters

**options** (`DistinctOptions`, optional)
    `dropnull` (default `true`) excludes `NULL` from the distinct set; set `false` to
    include it as its own distinct value.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog reduced to its distinct values.

## Examples

Example 1. Distinct values of a text stream and a numeric stream, in the same catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "north"})),
    "score"_k  = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.distinct();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 3       | [east, west, north]                              
score                    | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
========================================================================================================================
```

## Notes

- Each stream, in every catalog, is reduced to its own distinct values independently — the
  resulting streams within one catalog can end up different lengths.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::ndistinct`](ndistinct.md): the count of distinct values, instead of the values
  themselves.
- [`Archive::freq`](freq.md): distinct values paired with their occurrence counts.
- [`Catalog::distinct`](../catalog/distinct.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth.html">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="dloc.html">
    <span class="label">Next</span>
    <span class="title">dloc</span>
  </a>
</div>
