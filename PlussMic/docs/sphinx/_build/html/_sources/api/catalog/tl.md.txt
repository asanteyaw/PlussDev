<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 27e79b76df2e -->

# Catalog::tl

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogTimelineAccessor Catalog::tl() const
```

## What It Does

The entry point into timestamp-feature extraction on the catalog's `Timeline`-kind
(and `TimeIndex`-kind, for the applicable methods) streams. `tl()` binds the current
catalog and hands back a `CatalogTimelineAccessor`, which exposes 32 methods: calendar
field extraction (`year`, `month`, `day`, `hour`, ...), boolean predicates (`isnat`,
`valid`, `weekend`, `leap`), formatting/conversion (`fmt`, `iso`, `epoch`, `date`,
`time`), rounding (`floor`, `ceil`, `round`), and timezone/shift operations (`shift`,
`period`, `tz`, `localize`, `utc`, `index`) — see the
[CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)
for every method's own dedicated page.

```{important} tl() binds EVERY key in the catalog, not just the Timeline-kind ones
Same as [`Catalog::nt()`](nt.md) — `tl()` takes no arguments and always binds every
key currently in the catalog. Calling any CatalogTimelineAccessor method on a catalog
that has even one key that isn't `Timeline`- or `TimeIndex`-kind throws. Pre-filter
with [`Catalog::keys(names)`](keys.md) or [`Catalog::keyaxis().timeline()`](keyaxis.md)
first if you need to mix timestamp and non-timestamp processing.
```

## Parameters

`None`

## Returns

`CatalogTimelineAccessor`
    A binder for the current catalog — call one of its 32 methods to extract a
    calendar feature, reformat, or shift every `Timeline`-kind key.

## Examples

Example 1. `tl().year()` extracts the calendar year from every `Timeline`-kind key —
see [`CatalogTimelineAccessor::year`](../catalog-timeline/year.md) for this specific
method's own page.

```cpp
auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T09:10:00Z"})),
});
auto out = ct.tl().year();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [ts]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
ts                       | Series       | Series[int32]    | 2       | [2024, 2024]                                     
========================================================================================================================
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks every bound key's kind.
- See the important callout above — every one of the 32 methods on the returned
  accessor shares this every-key-bound, no-narrowing behavior.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise (checked
  when a method is actually called, not by `tl()` itself).
- Calendar-field-extraction methods (`year`, `month`, `day`, ...) return a `Series`-kind
  key in place of the original `Timeline`-kind key, per the output above — the
  catalog's key set and order are unchanged, only each bound key's own payload kind
  changes.

## See Also

- [CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.
- [`Catalog::nt`](nt.md) / [`Catalog::ts`](ts.md): the sibling accessors for
  text-processing and time-series operations.
- [`Catalog::keyaxis`](keyaxis.md): filter down to just the `Timeline`-kind keys first,
  via `.timeline()`.
- [`Registry::fields(keys).tl()`](../registry/tl.md): the scribble-model equivalent,
  which *does* let you narrow to a key subset before binding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tfidf.html">
    <span class="label">Previous</span>
    <span class="title">tfidf</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
