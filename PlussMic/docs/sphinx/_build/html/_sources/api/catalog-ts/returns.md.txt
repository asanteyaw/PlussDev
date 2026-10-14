<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 339c00a9a8da -->

# CatalogTimeSeriesAccessor::returns

[Back to CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimeSeriesAccessor::returns(std::int64_t periods = 1) const
```

## What It Does

Computes the fractional period-over-period change of every bound key:
`out[i] = x[i] / base[i] - 1`, where `base` is `x` shifted by `periods` (a lag
when `periods > 0`, a lead when `periods < 0`). A position is `NULL` wherever
`x[i]` or `base[i]` is missing, `base[i]` is `0`, or either value is
non-finite.

```{important} CatalogTimeSeriesAccessor requires every bound key to be numeric Series-kind
Same precondition as every method on this accessor — see
[`CatalogTimeSeriesAccessor::diff`](diff.md)'s own important callout for the
full explanation and how to project down to numeric keys first.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead). Must not be `0`, or the call throws.

## Returns

`Catalog`
    A new catalog with every bound key replaced by its period-over-period
    fractional return.

## Examples

Example 1. Compute the one-period return of a numeric key.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().returns(1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [nan, 1, -0.25, 1]                               
========================================================================================================================
```

Position `1` is `20 / 10 - 1 = 1` (a 100% increase); position `2` is
`15 / 20 - 1 = -0.25` (a 25% decrease); position `3` is `30 / 15 - 1 = 1`.

Example 2. Pass a negative `periods`, comparing each position against a
*later* one (a lead) instead of an earlier one.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().returns(-1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [-0.5, 0.333333333333, -0.5, nan]                
========================================================================================================================
```

With `periods = -1`, each position's `base` is the *next* position instead of
the previous one — the `nan` now lands at the opposite end (position `3`
instead of position `0`).

Example 3 (edge case). A `base` of exactly `0` comes back `NULL` rather than
infinity.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 20.0})),
});
auto out = ct.ts().returns(1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 3       | [nan, -1, nan]                                   
========================================================================================================================
```

Position `1` (`x = 0`, base `= 10`) is an ordinary `0 / 10 - 1 = -1`. Position
`2` (`x = 20`, base `= 0`) has a `base` of exactly `0` — rather than dividing
by zero, `returns()` reports `NULL` directly.

Example 4 (edge case). `periods = 0` throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
try {
  auto out = ct.ts().returns(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
subsp::stats::ts::returns: periods cannot be zero
```

Comparing each position against itself isn't a meaningful return, so
`periods = 0` is rejected outright rather than silently returning all-zero
output.

## Notes

- A `base` value of exactly `0` always produces `NULL`, regardless of `x[i]`,
  since the fractional change would otherwise be undefined or infinite.
- A pre-existing missing entry in a bound key is honored — it produces `NULL`
  in every return that would otherwise reference it.
- `periods` must not be `0`, or it throws
  `"subsp::stats::ts::returns: periods cannot be zero"`.
- See the important callout above.

## See Also

- [`CatalogTimeSeriesAccessor::log_returns`](log_returns.md): the logarithmic
  counterpart, additionally requiring both compared values to be strictly
  positive.
- [`CatalogTimeSeriesAccessor::diff`](diff.md): the absolute
  (non-fractional) period-over-period change.
- [`CatalogTimeSeriesAccessor::lag`](lag.md), [`lead()`](lead.md): the
  underlying shift this method's comparison base is built from.
- [`Catalog::ts`](../catalog/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::returns`](../slate-ts/returns.md): the underlying
  single-slate engine this method delegates to.

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
