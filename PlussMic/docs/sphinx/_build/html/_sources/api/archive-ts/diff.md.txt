<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc65c4435bc5 -->

# ArchiveTimeSeriesAccessor::diff

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::diff(std::int64_t periods = 1) const
```

## What It Does

Computes the absolute period-over-period change of every bound key, **within
each catalog independently**: `out[i] = x[i] - base[i]`, where `base` is `x`
shifted by `periods` (a lag when `periods > 0`, a lead when `periods < 0`). A
position is `NULL` wherever `x[i]` or `base[i]` is missing. `periods = 0`
produces an all-zero result rather than throwing. Delegates to each catalog's
own [`CatalogTimeSeriesAccessor::diff`](../catalog-ts/diff.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Each catalog's
diff is computed strictly from that catalog's own rows.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead).

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by its
    period-over-period absolute difference, in every catalog.

## Examples

Example 1. Absolute change at each position, with a leading `NaN` where there
is no prior value to compare against.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.ts().diff();
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
metric                   | Series       | Series[float64]  | 4       | [nan, 10, -5, 15]                                
========================================================================================================================
```

## Notes

- See the important callout above.
- Unlike [`returns()`](returns.md)/[`log_returns()`](log_returns.md),
  `periods = 0` is legal here — it produces an all-zero series rather than
  throwing.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::diff`](../catalog-ts/diff.md) directly on each
  catalog in the stack.

## See Also

- [`ArchiveTimeSeriesAccessor::lag`](lag.md) / [`lead()`](lead.md): the raw
  shift operation `diff()` builds its comparison base from.
- [`CatalogTimeSeriesAccessor::diff`](../catalog-ts/diff.md): the
  single-catalog delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="lag.html">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>
