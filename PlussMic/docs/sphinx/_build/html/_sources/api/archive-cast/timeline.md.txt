<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aff8aa010920 -->

# ArchiveCastAccessor::timeline

[Back to ArchiveCastAccessor API Reference](../../archive_cast_accessor_api_reference)

## Signature

```cpp
Archive ArchiveCastAccessor::timeline() const
```

## What It Does

Casts the bound key(s) to `Timeline`-kind (absolute timestamp) in **every catalog** of
the stack and returns a new archive with those keys replaced — one call per catalog,
delegating to that catalog's own
[`CatalogCastAccessor::timeline`](../catalog-cast/timeline.md).

```{important} Unparseable text becomes NaT, not a throw
Same as [`CatalogCastAccessor::timeline`](../catalog-cast/timeline.md): a `Note` value
that can't be parsed as a timestamp doesn't throw — it becomes `NaT` (not-a-time) in
the resulting key, in whichever catalog(s) contain it. Check
[`Archive::tl().isnat()`](../archive-timeline/isnat.md) after casting if you need to
detect values that failed to convert.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key cast to `Timeline`-kind in
    every catalog.

## Examples

Example 1. Cast a `Note`-kind key of date-looking text to `Timeline`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "d"_k = pluss::kindle::Stream(pluss::make_note({"2024-01-01", "2024-02-01"})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.cast().timeline();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [d]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
d                        | Timeline     | Timeline         | 2       | [2024-01-01T00:00:00.000~, 2024-02-01T00:00:00.0~
========================================================================================================================
```

## Notes

- Bound via [`Archive::cast()`](../archive/cast.md) — remember that entry point binds
  `catalog(0)`'s key set and requires every catalog to share it.
- See the important callout above for the silent-`NaT` behavior on unparseable text.

## See Also

- [`ArchiveCastAccessor::timeindex`](timeindex.md): the positional-index sibling
  target.
- [`CatalogCastAccessor::timeline`](../catalog-cast/timeline.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the richer accessor for feature extraction once a
  key is already `Timeline`-kind.
- [`Archive::cast`](../archive/cast.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
