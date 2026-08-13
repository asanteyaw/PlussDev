<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dddfbda3444d -->

# ArchiveTimeSeriesAccessor::rolling

[Back to ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimeSeriesAccessor::rolling(
    std::int64_t window,
    pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
    const pluss::table::WindowOptions& options = {}) const
```

## What It Does

Computes a **trailing rolling window statistic** over every bound key,
**within each catalog independently** — a window never spans across catalogs.
Each position's window is the `window` positions ending at (and including)
that position, within that position's own catalog; the first `window - 1`
positions in each catalog use a growing partial window rather than waiting
for a full one. Delegates to each catalog's own
[`CatalogTimeSeriesAccessor::rolling`](../catalog-ts/rolling.md).

```{important} Binding rules are inherited from Archive::ts()
The working key set is bound from `catalog(0)` and every catalog must share
it, per [`Archive::ts`](../archive/ts.md)'s important callouts. Only the
fixed-row-count overload exists here (same as
[`CatalogTimeSeriesAccessor::rolling`](../catalog-ts/rolling.md)) — no
calendar-duration form, since neither a catalog nor a stack of them has a
single unambiguous time axis to key one off of implicitly.
```

## Parameters

**window** (`std::int64_t`)
    The trailing window size, in positions, applied independently within each
    catalog. Must be positive, or it throws.

**statistic** (`pluss::table::WindowStatistic`, default `Mean`)
    The statistic computed over each window: `Sum`, `Mean`, `Minimum`,
    `Maximum`, `Var`, `Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const pluss::table::WindowOptions&`)
    `min_count` (default `1`), `ddof` (default `1`), `centered` (default
    `false`), `closure` (default `Right`) — see
    [`CatalogTimeSeriesAccessor::rolling`](../catalog-ts/rolling.md) for the
    full breakdown of each field's effect.

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by its
    rolling statistic at each position, in every catalog.

## Examples

Example 1. A fixed-size trailing rolling mean, applied independently to two
catalogs with different value ranges.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})) });
auto c1 = pluss::kindle::Catalog({ "metric"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.ts().rolling(2);
std::cout << out.catalog(0);
std::cout << out.catalog(1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [10, 15, 17.5, 22.5]                             
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [1, 1.5, 2.5, 3.5]                               
========================================================================================================================
```

Each catalog's rolling mean is computed purely from its own four values —
`c1`'s smaller-magnitude series doesn't influence `c0`'s window at all, and
vice versa.

## Notes

- See both important callouts above.
- The default options (`min_count = 1`, `centered = false`, `closure =
  Right`) mean the first `window - 1` positions in each catalog use a growing
  partial window rather than becoming `NULL`.
- This method's per-catalog result is identical to calling
  [`CatalogTimeSeriesAccessor::rolling`](../catalog-ts/rolling.md) directly on
  each catalog in the stack.

## See Also

- [`Stream::ts`](../stream/ts.md): the single-stream equivalent, which also
  exposes a calendar-duration overload.
- [`CatalogTimeSeriesAccessor::rolling`](../catalog-ts/rolling.md): the
  single-catalog delegate this method applies to every catalog.
- [`Archive::ts`](../archive/ts.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="returns.html">
    <span class="label">Previous</span>
    <span class="title">returns</span>
  </a>
</div>
