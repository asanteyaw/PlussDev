<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b8b72132a3b3 -->

# ArchiveTimeSeriesAccessor::returns

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::returns(std::int64_t periods = 1) const
```

## What It Does

Computes the fractional period-over-period change of every bound key, **within
each catalog independently**: `out[i] = x[i] / base[i] - 1`, where `base` is
`x` shifted by `periods` (a lag when `periods > 0`, a lead when `periods <
0`). A position is `NULL` wherever `x[i]` or `base[i]` is missing, `base[i]`
is `0`, or either value is non-finite. Delegates to each catalog's own
[`CatalogTimeSeriesAccessor::returns`](../catalog-ts/returns.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Each catalog's
returns are computed strictly from that catalog's own rows — there is no
cross-catalog (cross-depth) comparison.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead). Must not be `0`, or the call throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by its
    period-over-period fractional return, in every catalog.

## Examples

Example 1. Two catalogs with different price paths — `returns()` computes
each one's own fractional change independently.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({100.0, 110.0, 121.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({50.0, 45.0, 54.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.ts().returns();
std::cout << out.catalog(0);
std::cout << out.catalog(1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 3       | [nan, 0.1, 0.1]                                  
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [px]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
px                       | Series       | Series[float64]  | 3       | [nan, -0.1, 0.2]                                 
========================================================================================================================
```

`c1`'s dip-then-recovery produces a `-10%` then `+20%` return — computed
entirely from `c1`'s own three values, unaffected by `c0`.

## Notes

- See the important callout above.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::returns`](../catalog-ts/returns.md) directly on
  each catalog in the stack.

## See Also

- [`ArchiveTimeSeriesAccessor::log_returns`](log_returns.md): the logarithmic
  variant.
- [`CatalogTimeSeriesAccessor::returns`](../catalog-ts/returns.md): the
  single-catalog delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log_returns.html">
    <span class="label">Previous</span>
    <span class="title">log_returns</span>
  </a>
  <a class="next-link" href="rolling.html">
    <span class="label">Next</span>
    <span class="title">rolling</span>
  </a>
</div>
