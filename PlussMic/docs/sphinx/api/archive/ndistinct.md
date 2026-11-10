<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e3a139f0908 -->

# Archive::ndistinct

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::ndistinct(DistinctOptions options = {}) const -> Archive
```

### `DistinctOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |

## What It Does

Returns a new archive where every catalog is replaced by its own
[`Catalog::ndistinct()`](../catalog/ndistinct.md) result — each stream reduced to a single
count of its distinct values, per catalog. `options.dropnull` (default `true`) controls
whether `NULL` counts as one of the distinct values.

## Parameters

**options** (`DistinctOptions`, optional)
    `dropnull` (default `true`) excludes `NULL` from the distinct count; set `false` to
    include it.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog reduced to a one-element `Series[int64]` holding its distinct-value count.

## Examples

Example 1. Count of distinct values in a text stream.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "north"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.ndistinct();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [region]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Series       | Series[int64]    | 1       | [3]                                              
========================================================================================================================
```

## Notes

- Each stream, in every catalog, is reduced independently to a one-element result.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::distinct`](distinct.md): the distinct values themselves, instead of just the
  count.
- [`Archive::freq`](freq.md): distinct values paired with their occurrence counts.
- [`Catalog::ndistinct`](../catalog/ndistinct.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minrecords.html">
    <span class="label">Previous</span>
    <span class="title">minrecords</span>
  </a>
  <a class="next-link" href="nkeys.html">
    <span class="label">Next</span>
    <span class="title">nkeys</span>
  </a>
</div>
