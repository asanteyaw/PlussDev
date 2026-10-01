<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 682ff91d4e09 -->

# ArchiveKeyAxisAccessor::note

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::note() const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
its `Note`-kind keys — each catalog filtered independently, by delegating to that
catalog's own [`CatalogKeyAxisAccessor::note`](../catalog-keyaxis/note.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `note()` never validates keys across catalogs. If one
catalog has two `Note`-kind keys and another has none, the result simply has an empty
catalog at that depth position — no throw.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own `Note`-kind
    keys.

## Examples

Example 1. Narrow a catalog with a mix of `Series` and `Note` keys down to just the
`Note`-kind one.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "n"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "s"_k = pluss::kindle::Stream(pluss::make_note({"x"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().note();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [s]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
s                        | Note         | Note             | 1       | [x]                                              
========================================================================================================================
```

## Notes

- See the important callout above.
- A catalog with zero `Note`-kind keys becomes a valid, defined, empty catalog in the
  result.

## See Also

- [`ArchiveKeyAxisAccessor::series`](series.md) / [`timeline()`](timeline.md) /
  : the sibling kind filters.
- [`CatalogKeyAxisAccessor::note`](../catalog-keyaxis/note.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="prefix.html">
    <span class="label">Next</span>
    <span class="title">prefix</span>
  </a>
</div>
