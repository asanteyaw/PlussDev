<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 449f94bdbb5c -->

# Catalog::ts

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogTimeSeriesAccessor Catalog::ts() const
```

## What It Does

The entry point into time-series transformations on the catalog's numeric streams.
`ts()` binds the current catalog and hands back a `CatalogTimeSeriesAccessor`, which
exposes 6 methods: `returns`, `log_returns`, `diff`, `lag`, `lead` (all
period-shift-based transforms), and `rolling` (windowed statistics) — see the
[CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference)
for every method's own dedicated page.

```{important} ts() binds EVERY key in the catalog, not just numeric ones
Same pattern as [`Catalog::nt()`](nt.md)/[`Catalog::tl()`](tl.md) — `ts()` takes no
arguments and always binds every key currently in the catalog. Calling any
CatalogTimeSeriesAccessor method on a catalog with a non-numeric key throws. Pre-filter
with [`Catalog::keys(names)`](keys.md) first if you need to mix time-series and
non-numeric processing.
```

```{important} CatalogTimeSeriesAccessor is a small subset of StreamTimeSeriesAccessor
Catalog::ts() exposes only 6 methods -- the period-shift family plus one basic
rolling() overload. Stream::ts() (the single-stream equivalent) exposes a much larger
surface: expanding(), ewm(), zscore_rolling(), fill_gaps(), align_to(), lag_block(),
rolling_block(), calendar(), sort(), and axis-driven rolling()/timed()/grouped()
overloads that need a companion time axis -- none of which have a whole-catalog
equivalent here, since those operations need a single, unambiguous time axis to key
off of, which a multi-key catalog doesn't have implicitly. Apply
[`Catalog::keys(key)`](keys.md) to get down to `Stream::ts()` for the richer surface.
```

## Parameters

`None`

## Returns

`CatalogTimeSeriesAccessor`
    A binder for the current catalog — call one of its 6 methods to shift or
    window-aggregate every numeric key.

## Examples

Example 1. `ts().lag(1)` shifts every numeric key's values forward by one position,
introducing a leading `NaN` — see
[`CatalogTimeSeriesAccessor::lag`](../catalog-ts/lag.md) for this specific method's
own page.

```cpp
auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});
auto out = ct.ts().lag(1);
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
metric                   | Series       | Series[float64]  | 3       | [nan, 10, 20]                                    
========================================================================================================================
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks every bound key's kind.
- See the important callouts above — every one of the 6 methods on the returned
  accessor shares this every-key-bound, no-narrowing behavior, and the accessor's
  surface is intentionally smaller than `Stream::ts()`'s own.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise (checked
  when a method is actually called, not by `ts()` itself).

## See Also

- [CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference):
  the full list of 6 methods this accessor exposes.
- [`Catalog::nt`](nt.md) / [`Catalog::tl`](tl.md): the sibling accessors for
  text-processing and timestamp-feature operations.
- [`Stream::ts`](../stream/ts.md): the single-stream equivalent, with a much larger
  method surface — see the important callout above.
- [`Registry::fields(keys).tl()`](../registry/tl.md): the bundle-model's equivalent
  narrowing pattern (Registry's bundle model has no direct `ts()` counterpart).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
