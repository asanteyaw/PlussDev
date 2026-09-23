<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74bc79649a5d -->

# Catalog::ndistinct

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::ndistinct(DistinctOptions options = {}) const -> Catalog
```

## What It Does

Counts the number of distinct values in **every** stream, independently — each stream's
own `ndistinct()`, collapsing each one to a single-value count. Unlike
[`distinct()`](distinct.md) and [`freq()`](freq.md), the result here is *not* ragged in the
usual worrying sense: every stream ends up with exactly one row (its own distinct-value
count), so the resulting catalog is uniformly single-row.

## Parameters

**options** (`DistinctOptions`, optional)
    `dropnull` (default `true`) excludes `NULL` from each stream's distinct-value count.

## Returns

`Catalog`
    A new catalog with every stream replaced by a single-value count of its own distinct
    values.

## Examples

Example 1. Count distinct values per stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "east"})),
});

auto out = ct.ndistinct();
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
id                       | Series       | Series[int64]    | 1       | [3]                                              
region                   | Series       | Series[int64]    | 1       | [2]                                              
========================================================================================================================
```

`id` has 3 distinct values (`1`, `2`, `3`); `region` has 2 (`"east"`, `"west"`).

## Notes

- Unlike [`distinct()`](distinct.md)/[`freq()`](freq.md), the result is uniformly
  single-row per stream, not ragged — this is a count, not a list of values.
- `dropnull` (default `true`) excludes `NULL` from the count.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::distinct`](distinct.md): the distinct values themselves, rather than just
  their count.
- [`Catalog::freq`](freq.md): per-value occurrence counts, rather than a single
  distinct-value total.
- [`Catalog::nkeys`](nkeys.md): the number of *streams*, a different question from the
  number of distinct *values* within one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minrows.html">
    <span class="label">Previous</span>
    <span class="title">minrows</span>
  </a>
  <a class="next-link" href="nkeys.html">
    <span class="label">Next</span>
    <span class="title">nkeys</span>
  </a>
</div>
