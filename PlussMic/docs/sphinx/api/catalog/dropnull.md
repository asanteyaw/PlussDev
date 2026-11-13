<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7b7aebda7e6 -->

# Catalog::dropnull

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::dropnull() const
```

## What It Does

Drops every row that is `NULL` in **any** stream — combining each stream's own validity
mask with a logical AND across the whole catalog, then filtering every stream by that
combined mask. A row survives only if it's valid in every single stream; a row with a
`NULL` in just one stream is dropped from all of them, keeping every stream aligned to the
same reduced row set.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every stream restricted to the rows that were valid across every
    other stream too.

## Examples

Example 1. A row that's `NULL` in either stream is dropped from both.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.dropnull();
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
id                       | Series       | Series[float64]  | 1       | [1]                                              
city                     | Note         | Note             | 1       | [Accra]                                          
========================================================================================================================
```

Only row `0` is valid in both `id` (rows `1` and `2` have one invalid entry each between
the two streams) — rows `1` and `2` are dropped entirely, from every stream, even though
each was only `NULL` in *one* of the two streams.

## Notes

- A row is kept only if it's valid in **every** stream — a single `NULL` anywhere in that
  row drops it from the whole catalog.
- Every stream stays aligned to the same reduced row set afterward.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::fillnull`](fillnull.md): fill `NULL` cells instead of dropping their rows.
- [`Catalog::isnull`](isnull.md) / [`Catalog::notnull`](notnull.md): inspect per-cell
  validity without dropping any rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
