<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aff8aa010920 -->

# ArchiveKeyAxisAccessor::timeline

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::timeline() const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
its `Timeline`-kind keys — each catalog filtered independently, by delegating to that
catalog's own [`CatalogKeyAxisAccessor::timeline`](../catalog-keyaxis/timeline.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `timeline()` never validates keys across catalogs. A
ragged archive (different key sets per catalog) works without throwing, and the result
can end up just as ragged.
```

```{important} Timeline is distinct from TimeIndex
`Timeline`-kind (an absolute timestamp axis) and `TimeIndex`-kind (a positional/period
index) are different payload kinds with separate filters here — see
for the sibling method. A `TimeIndex`-kind key is
excluded from `timeline()`'s result, and vice versa.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    `Timeline`-kind keys.

## Examples

Example 1. A catalog with a mix of `Series` and `Timeline` keys is narrowed down to
just the `Timeline`-kind one.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "n"_k  = pluss::kindle::Stream(pluss::make_series({1.0})),
    "dt"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01", "2024-02-01"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().timeline();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [dt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
dt                       | Timeline     | Timeline         | 2       | [2024-01-01T00:00:00, 2024-02-01T00:00:00]      
========================================================================================================================
```

## Notes

- See both important callouts above.
- A catalog with zero `Timeline`-kind keys becomes a valid, defined, empty catalog in
  the result.

## See Also

- [`ArchiveKeyAxisAccessor::series`](series.md) / [`note()`](note.md): the other kind
  filters.
- [`CatalogKeyAxisAccessor::timeline`](../catalog-keyaxis/timeline.md): the
  single-catalog delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.
- [`Archive::tl`](../archive/tl.md): a richer accessor for `Timeline`-kind (and
  `TimeIndex`-kind) feature extraction, once you already know a key is timestamp-like.

<div class="pluss-inline-prev-next">
</div>
