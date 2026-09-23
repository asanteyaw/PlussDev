<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4dbc7d06a045 -->

# ArchiveTimeSeriesAccessor::log_returns

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::log_returns(std::int64_t periods = 1) const
```

## What It Does

Computes the logarithmic period-over-period change of every bound key,
**within each catalog independently**: `out[i] = ln(x[i] / base[i])`, where
`base` is `x` shifted by `periods` (a lag when `periods > 0`, a lead when
`periods < 0`). A position is `NULL` wherever `x[i]` or `base[i]` is missing
or non-finite, **or either value is not strictly positive** — unlike
[`returns()`](returns.md), a zero or negative value anywhere in the
comparison always yields `NULL`. Delegates to each catalog's own
[`CatalogTimeSeriesAccessor::log_returns`](../catalog-ts/log_returns.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Each catalog's
log-returns are computed strictly from that catalog's own rows.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead). Must not be `0`, or the call throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by its
    period-over-period log return, in every catalog.

## Examples

Example 1. A constant 10%-per-step growth catalog produces the same log
return at every eligible position.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({100.0, 110.0, 121.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.ts().log_returns();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 3       | [nan, 0.0953101798043, 0.0953101798043]          
========================================================================================================================
```

## Notes

- See the important callout above.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::log_returns`](../catalog-ts/log_returns.md)
  directly on each catalog in the stack.

## See Also

- [`ArchiveTimeSeriesAccessor::returns`](returns.md): the fractional (not
  logarithmic) variant.
- [`CatalogTimeSeriesAccessor::log_returns`](../catalog-ts/log_returns.md):
  the single-catalog delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lead.html">
    <span class="label">Previous</span>
    <span class="title">lead</span>
  </a>
  <a class="next-link" href="returns.html">
    <span class="label">Next</span>
    <span class="title">returns</span>
  </a>
</div>
