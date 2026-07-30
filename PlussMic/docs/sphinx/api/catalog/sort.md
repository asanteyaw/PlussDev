<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 79c7e3e6e8e5 -->

# Catalog::sort

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::sort(std::string_view by_field, bool ascending = true) const -> Catalog
```

## What It Does

Reorders every stream's rows by the values in one named stream — a single-key convenience
wrapper over [`sort_by({by_field}, {ascending})`](sort_by.md). Every stream in the catalog
is reordered together, keeping row alignment intact across the whole catalog, not just the
sort key itself.

## Parameters

**by_field** (`std::string_view`)
    The stream name to sort by. Must exist.

**ascending** (`bool`, default `true`)
    `true` sorts smallest-to-largest (or alphabetically for text); `false` reverses the
    order.

## Returns

`Catalog`
    A new catalog with every stream's rows reordered by `by_field`'s values.

## Examples

Example 1. Sort ascending (the default) by a numeric stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({3.0, 1.0, 2.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 10.0, 20.0})),
});

auto out = ct.sort("id");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

`metric`'s rows moved along with `id`'s — the whole catalog stays aligned to the new row
order, not just the sort key.

Example 2. Sort descending with `ascending = false`.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({3.0, 1.0, 2.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 10.0, 20.0})),
});

auto out = ct.sort("id", false);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [3, 2, 1]                                        
metric                   | Series       | Series[float64]  | 3       | [30, 20, 10]                                     
========================================================================================================================
```

## Notes

- A convenience wrapper over [`sort_by()`](sort_by.md) with one key.
- Reorders *every* stream together, preserving cross-stream row alignment.
- `by_field` must exist; throws otherwise.

## See Also

- [`Catalog::sort_by`](sort_by.md): the multi-key form this method is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sort_by.html">
    <span class="label">Next</span>
    <span class="title">sort_by</span>
  </a>
</div>
