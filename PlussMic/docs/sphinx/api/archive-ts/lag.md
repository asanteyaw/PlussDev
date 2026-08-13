<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81cff3684790 -->

# ArchiveTimeSeriesAccessor::lag

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::lag(std::int64_t periods = 1) const
```

## What It Does

Shifts every bound key `periods` positions forward, **within each catalog
independently**: `out[i] = x[i - periods]` where that index is in bounds,
`NULL` otherwise. A negative `periods` behaves as [`lead(-periods)`](lead.md).
Delegates to each catalog's own
[`CatalogTimeSeriesAccessor::lag`](../catalog-ts/lag.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Each catalog
is shifted using only its own rows — a catalog's leading position never pulls
in a trailing value from a different catalog in the stack.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift forward. Negative values behave as
    [`lead(-periods)`](lead.md) instead.

## Returns

`Archive`
    A new archive of the same depth, with every bound key shifted by
    `periods` positions, in every catalog.

## Examples

Example 1. Lag a numeric key by one position; position `0` has no
predecessor, so it's `nan`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.ts().lag(1);
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [nan, 10, 20, 15]                                
========================================================================================================================
```

## Notes

- See the important callout above.
- `lag(periods)` with a negative `periods` is exactly `lead(-periods)`.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::lag`](../catalog-ts/lag.md) directly on each
  catalog in the stack.

## See Also

- [`ArchiveTimeSeriesAccessor::lead`](lead.md): the mirror-image shift.
- [`CatalogTimeSeriesAccessor::lag`](../catalog-ts/lag.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="lead.html">
    <span class="label">Next</span>
    <span class="title">lead</span>
  </a>
</div>
