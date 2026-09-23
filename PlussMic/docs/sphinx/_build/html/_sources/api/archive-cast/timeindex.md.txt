<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b34bfba8b98c -->

# ArchiveCastAccessor::timeindex

[Back to ArchiveCastAccessor API Reference](../../archive_cast_accessor_api_reference)

## Signature

```cpp
Archive ArchiveCastAccessor::timeindex() const
```

## What It Does

Casts the bound key(s) to `TimeIndex`-kind (positional/period index) in **every
catalog** of the stack and returns a new archive with those keys replaced — one call
per catalog, delegating to that catalog's own
[`CatalogCastAccessor::timeindex`](../catalog-cast/timeindex.md).

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key cast to `TimeIndex`-kind in
    every catalog.

## Examples

Example 1. Cast a numeric key to a `TimeIndex`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "n"_k = pluss::kindle::Stream(pluss::make_series({0.0, 1.0, 2.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.cast().timeindex();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [n]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
n                        | TimeIndex    | TimeIndex        | 3       | [1970-01-01T00:00:00.000~, 1970-01-01T00:00:00.0~
========================================================================================================================
```

## Notes

- Bound via [`Archive::cast()`](../archive/cast.md) — remember that entry point binds
  `catalog(0)`'s key set and requires every catalog to share it.
- Follows the same numeric-to-`TimeIndex` conversion rules as
  [`CatalogCastAccessor::timeindex`](../catalog-cast/timeindex.md) —
  see that page for the full breakdown of how numeric values map onto positions.

## See Also

- [`ArchiveCastAccessor::timeline`](timeline.md): the absolute-timestamp sibling
  target.
- [`CatalogCastAccessor::timeindex`](../catalog-cast/timeindex.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::cast`](../archive/cast.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
