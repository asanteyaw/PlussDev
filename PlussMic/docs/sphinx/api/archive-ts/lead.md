<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55deaa7f77cf -->

# ArchiveTimeSeriesAccessor::lead

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::lead(std::int64_t periods = 1) const
```

## What It Does

Shifts every bound key `periods` positions backward, **within each catalog
independently**: `out[i] = x[i + periods]` where that index is in bounds,
`NULL` otherwise. A negative `periods` behaves as [`lag(-periods)`](lag.md).
Delegates to each catalog's own
[`CatalogTimeSeriesAccessor::lead`](../catalog-ts/lead.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Each catalog
is shifted using only its own rows — a catalog's trailing position never
pulls in a leading value from a different catalog in the stack.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift backward. Negative values behave as
    [`lag(-periods)`](lag.md) instead.

## Returns

`Archive`
    A new archive of the same depth, with every bound key shifted by
    `periods` positions, in every catalog.

## Examples

Example 1. `lag(-1)` and `lead(1)` produce identical results, confirming the
mirror-image relationship documented above.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out_lag = archive.ts().lag(-1);
auto out_lead = archive.ts().lead(1);
std::cout << "lag(-1):\n" << out_lag.catalog(0);
std::cout << "lead(1):\n" << out_lead.catalog(0);
```

### Output

```text
lag(-1):
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [20, 15, 30, nan]                                
========================================================================================================================
lead(1):
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [20, 15, 30, nan]                                
========================================================================================================================
```

## Notes

- See the important callout above.
- `lead(periods)` with a negative `periods` is exactly `lag(-periods)`.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::lead`](../catalog-ts/lead.md) directly on each
  catalog in the stack.

## See Also

- [`ArchiveTimeSeriesAccessor::lag`](lag.md): the mirror-image shift.
- [`CatalogTimeSeriesAccessor::lead`](../catalog-ts/lead.md): the
  single-catalog delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="log_returns.html">
    <span class="label">Next</span>
    <span class="title">log_returns</span>
  </a>
</div>
