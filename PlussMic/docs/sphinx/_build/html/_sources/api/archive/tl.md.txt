<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b639103eb64 -->

# Archive::tl

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
ArchiveTimelineAccessor Archive::tl() const
```

## What It Does

The entry point into timestamp-feature extraction across the whole depth stack.
`tl()` binds the current archive and hands back an `ArchiveTimelineAccessor`, which
exposes the same 32 methods as [`CatalogTimelineAccessor`](../catalog/tl.md) — calendar
field extraction (`year`, `month`, `day`, `hour`, ...), boolean predicates (`isnat`,
`valid`, `weekend`, `leap`), formatting/conversion (`fmt`, `iso`, `epoch`, `date`,
`time`), rounding (`floor`, `ceil`, `round`), and timezone/shift operations (`shift`,
`period`, `tz`, `localize`, `utc`, `index`) — see the
[ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)
for every method's own dedicated page. Every method broadcasts across **every catalog**
in the stack, returning a new `Archive` of the same depth.

```{important} tl() binds catalog(0)'s key set — not each catalog's own keys
Unlike [`Catalog::tl()`](../catalog/tl.md), which always binds *every key currently in
that one catalog*, `Archive::tl()` reads its working key set from the **first catalog
in the stack** (`catalog(0).keys()`) and then requires every other catalog to also
carry each of those exact key names. If any catalog in the stack is missing one of
catalog(0)'s keys, `tl()` itself throws immediately — before you call any method on the
returned accessor — with the same `"Archive::keys: unknown key '...'"` message
[`Archive::keys(names)`](keys.md) uses for an unrecognized key. An archive with a
uniform key set across all its catalogs (the common case) is unaffected by this.
```

```{important} Kind mismatch is checked lazily, per method call
Binding the key set (the callout above) happens eagerly, at `tl()` call time. Whether
each bound key is actually `Timeline`- or `TimeIndex`-kind is **not** checked until you
call a specific method on the returned accessor — same as
[`Catalog::tl()`](../catalog/tl.md). `archive.tl()` alone never throws for a wrong-kind
stream; `archive.tl().year()` does, the moment the wrong-kind key is reached in any
catalog.
```

## Parameters

`None`

## Returns

`ArchiveTimelineAccessor`
    A binder for the current archive's stack — call one of its 32 methods to extract a
    calendar feature, reformat, or shift every catalog's copy of the bound keys.

## Examples

Example 1. `tl().year()` extracts the calendar year from a `Timeline`-kind key,
uniformly in every catalog of the stack.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "dt"_k = pluss::kindle::Stream(pluss::make_timeline({"2020-01-15", "2020-06-20"})),
});
auto c1 = pluss::kindle::Catalog({
    "dt"_k = pluss::kindle::Stream(pluss::make_timeline({"2021-03-01", "2021-09-10"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.tl().year();
std::cout << out.catalog(0);
std::cout << out.catalog(1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [dt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
dt                       | Series       | Series[int32]    | 2       | [2020, 2020]                                     
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [dt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
dt                       | Series       | Series[int32]    | 2       | [2021, 2021]                                     
========================================================================================================================
```

Example 2. A ragged stack — `c1` is missing the `extra` key that `c0` has — throws
immediately from `tl()` itself, per the first important callout above.

```cpp
using namespace pluss::kindle::literals;

auto dt_stream = pluss::kindle::Stream(pluss::make_timeline({"2020-01-15", "2020-06-20"}));
auto c0 = pluss::kindle::Catalog({
    "dt"_k    = dt_stream,
    "extra"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto c1 = pluss::kindle::Catalog({
    "dt"_k = dt_stream,
});
auto archive = pluss::kindle::Archive({c0, c1});

try {
  auto out = archive.tl();
} catch (const std::exception& e) {
  std::cout << "threw: " << e.what() << "\n";
}
```

### Output

```text
threw: Archive::keys: unknown key 'extra'
```

## Notes

- See both important callouts above — the key-set binding is eager and stack-wide;
  the payload-kind check is lazy and per-method.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise (checked
  when a method is actually called, not by `tl()` itself).
- Calendar-field-extraction methods (`year`, `month`, `day`, ...) return a `Series`-kind
  key in place of the original `Timeline`-kind key in each catalog, per the output
  above — every catalog's key set and order are otherwise unchanged.
- On an empty archive (`depth() == 0`), `tl()` binds an empty key set and never throws;
  every method on the returned accessor is a no-op that returns another empty archive.

## See Also

- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.
- [`Catalog::tl`](../catalog/tl.md): the single-catalog equivalent this method's
  per-catalog broadcast delegates to.
- [`Archive::ts`](ts.md): the sibling accessor for time-series transformations, with the
  same catalog(0)-keys binding rule.
- [`Archive::keys`](keys.md): narrow the archive to a uniform key subset first if your
  stack is ragged and you only need a subset of catalogs' shared keys.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
