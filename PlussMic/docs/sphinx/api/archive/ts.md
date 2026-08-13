<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e987523bf266 -->

# Archive::ts

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
ArchiveTimeSeriesAccessor Archive::ts() const
```

## What It Does

The entry point into time-series transformations across the whole depth stack. `ts()`
binds the current archive and hands back an `ArchiveTimeSeriesAccessor`, which exposes
the same 6 methods as [`CatalogTimeSeriesAccessor`](../catalog/ts.md): `returns`,
`log_returns`, `diff`, `lag`, `lead` (all period-shift-based transforms), and `rolling`
(windowed statistics) — see the
[ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference) for
every method's own dedicated page. Every method broadcasts across **every catalog** in
the stack independently, returning a new `Archive` of the same depth.

```{important} ts() binds catalog(0)'s key set — not each catalog's own keys
Same rule as [`Archive::tl()`](tl.md): the working key set comes from **the first
catalog in the stack** (`catalog(0).keys()`), and every other catalog must carry each
of those exact key names or `ts()` itself throws immediately, with
`"Archive::keys: unknown key '...'"`. Whether each bound key is actually numeric is
checked lazily, the moment you call a specific method on the returned accessor — not by
`ts()` itself.
```

```{important} Each catalog's series is shifted/windowed independently
Time-series transforms operate strictly within a single catalog's own rows — there is
no cross-catalog (cross-depth) shifting or windowing here. `ts().lag(1)` shifts each
catalog's own values by one row *within that catalog*; catalog 1's leading row does not
pull in a trailing value from catalog 0. If you need a shift across the *depth* axis
itself, there is no direct equivalent — [`Archive::slice_depth`](slice_depth.md) and
manual catalog indexing are the closest tools.
```

## Parameters

`None`

## Returns

`ArchiveTimeSeriesAccessor`
    A binder for the current archive's stack — call one of its 6 methods to shift or
    window-aggregate every catalog's copy of the bound keys.

## Examples

Example 1. `ts().returns()` computes simple percent returns independently within each
catalog — the leading `NaN` appears in *every* catalog, not just the first.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "px"_k = pluss::kindle::Stream(pluss::make_series({100.0, 110.0, 121.0})),
});
auto c1 = pluss::kindle::Catalog({
    "px"_k = pluss::kindle::Stream(pluss::make_series({50.0, 55.0, 60.5})),
});
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
px                       | Series       | Series[float64]  | 3       | [nan, 0.1, 0.1]                                  
========================================================================================================================
```

Both catalogs happen to have a constant 10%-per-step growth rate here, which is why
their `returns()` outputs look identical — each was computed independently from that
catalog's own three rows, per the second important callout above.

## Notes

- See both important callouts above — the key-set binding is eager and stack-wide,
  numeric-kind checking is lazy and per-method, and every transform stays within its
  own catalog's rows.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise (checked
  when a method is actually called, not by `ts()` itself).
- `ArchiveTimeSeriesAccessor`'s 6-method surface mirrors
  [`CatalogTimeSeriesAccessor`](../catalog/ts.md) exactly — it does not expose
  `Stream::ts()`'s richer surface (`expanding`, `ewm`, `align_to`, ...) for the same
  reason `Catalog::ts()` doesn't: those operations need a single, unambiguous time axis
  that a multi-key catalog (or a stack of them) doesn't have implicitly.
- On an empty archive (`depth() == 0`), `ts()` binds an empty key set and never throws;
  every method on the returned accessor is a no-op that returns another empty archive.

## See Also

- [ArchiveTimeSeriesAccessor API Reference](../../archive_ts_accessor_api_reference):
  the full list of 6 methods this accessor exposes.
- [`Catalog::ts`](../catalog/ts.md): the single-catalog equivalent this method's
  per-catalog broadcast delegates to.
- [`Archive::tl`](tl.md): the sibling accessor for timestamp-feature extraction, with
  the same catalog(0)-keys binding rule.
- [`Archive::keys`](keys.md): narrow the archive to a uniform key subset first if your
  stack is ragged and you only need a subset of catalogs' shared keys.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
