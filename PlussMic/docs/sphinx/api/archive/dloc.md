<!-- pluss-hand-authored -->

# Archive::dloc

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::dloc(std::int64_t i) const -> Catalog
```

## What It Does

An exact synonym for [`catalog(i)`](catalog.md) — implemented as a direct forwarding call
(`return catalog(i);`). Use whichever name reads better at the call site; `dloc` echoes the
depth-axis wording used elsewhere on this class (`slice_depth`, `aggregate_over_catalogs`).

## Parameters

**i** (`std::int64_t`)
    The depth-index to fetch. Negative values count from the end, same as
    [`catalog()`](catalog.md).

## Returns

`Catalog`
    The catalog at depth-index `i`.

## Examples

Example 1. `dloc()` and `catalog()` return the identical result for the same index.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.dloc(1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

## Notes

- Identical behavior to [`catalog()`](catalog.md) in every respect, including negative
  indexing and the same `"Archive: index out of bounds"` throw.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::catalog`](catalog.md): the exact synonym this method forwards to.
- [`Archive::loc`](loc.md): fetch a catalog by depth *label* instead of positional index.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
