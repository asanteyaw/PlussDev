<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f784c6a070b4 -->

# CatalogTimeSeriesAccessor::diff

[Back to CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimeSeriesAccessor::diff(std::int64_t periods = 1) const
```

## What It Does

Computes the absolute period-over-period change of every bound key:
`out[i] = x[i] - base[i]`, where `base` is `x` shifted by `periods` (a lag when
`periods > 0`, a lead when `periods < 0`). A position is `NULL` wherever `x[i]`
or `base[i]` is missing. `periods = 0` produces an all-zero result rather than
throwing.

```{important} CatalogTimeSeriesAccessor requires every bound key to be numeric Series-kind
Every method on this accessor routes through `Stream::ts()` for each bound key.
If any bound key isn't a numeric `Series` (`Note`, `Timeline`, `TimeIndex`, or a
non-numeric `Series`), the call throws. Since [`Catalog::ts()`](../catalog/ts.md)
binds *every* key of the catalog by default, project down to just the numeric
keys first with [`Catalog::keys(names)`](../catalog/keys.md) or
[`Catalog::keyaxis().series()`](../catalog/keyaxis.md) before calling `.ts()`.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead).

## Returns

`Catalog`
    A new catalog with every bound key replaced by its period-over-period
    absolute difference.

## Examples

Example 1. Compute the one-period difference of a numeric key.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().diff(1);
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
metric                   | Series       | Series[float64]  | 4       | [nan, 10, -5, 15]                                
========================================================================================================================
```

Position `0` has no prior position, so it's `nan`. Position `1` is
`20 - 10 = 10`; position `2` is `15 - 20 = -5`; position `3` is `30 - 15 = 15`.

Example 2. `periods = 0` produces an all-zero result rather than throwing.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().diff(0);
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
metric                   | Series       | Series[float64]  | 4       | [0, 0, 0, 0]                                     
========================================================================================================================
```

Unlike a hand-rolled shift-based approach, `diff()` handles `periods = 0`
explicitly by returning an all-zero identity comparison rather than a key of
`NULL`s.

Example 3 (edge case). A non-numeric key throws — see the important callout
above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "name"_k = pluss::kindle::Stream(pluss::make_note({"a", "b"})),
});
try {
  auto out = ct.ts().diff(1);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeSeriesAccessor::diff: source must be Series
```

## Notes

- Unlike a hand-rolled shift-based approach you might build by hand, `diff()`
  handles `periods = 0` explicitly by returning all zeros (an identity
  comparison) rather than a key of `NULL`s.
- A pre-existing missing entry in a bound key is honored — it produces `NULL`
  in every difference that would otherwise reference it, not just the `NULL`s
  the shift itself introduces at the vacated end.
- See the important callout above.

## See Also

- [`CatalogTimeSeriesAccessor::returns`](returns.md),
  [`log_returns()`](log_returns.md): the fractional and logarithmic
  counterparts to this absolute difference.
- [`CatalogTimeSeriesAccessor::lag`](lag.md), [`lead()`](lead.md): the
  underlying shift this method's comparison base is built from.
- [`Catalog::ts`](../catalog/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::diff`](../slate-ts/diff.md): the underlying
  single-slate engine this method delegates to, with additional edge-case
  examples (missing-entry propagation).

<div class="pluss-inline-prev-next">
  <a class="next-link" href="lag.html">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>
