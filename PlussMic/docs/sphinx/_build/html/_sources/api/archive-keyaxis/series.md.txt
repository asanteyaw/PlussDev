<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bbecc9a7400b -->

# ArchiveKeyAxisAccessor::series

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::series() const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
its `Series`-kind (numeric) keys — each catalog filtered independently, by delegating
to that catalog's own [`CatalogKeyAxisAccessor::series`](../catalog-keyaxis/series.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `series()` never validates keys across catalogs. If
one catalog has three numeric keys and another has none, the result simply has an
empty catalog at that depth position — no throw.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    `Series`-kind keys.

## Examples

Example 1. Narrow a catalog with a mix of `Series` and `Note` keys down to just the
numeric one.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "n"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "s"_k = pluss::kindle::Stream(pluss::make_note({"x"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().series();
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
n                        | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
```

## Notes

- See the important callout above.
- A catalog with zero `Series`-kind keys becomes a valid, defined, empty catalog in the
  result.

## See Also

- [`ArchiveKeyAxisAccessor::note`](note.md) / [`timeline()`](timeline.md) /
  [`timeindex()`](timeindex.md): the sibling kind filters.
- [`CatalogKeyAxisAccessor::series`](../catalog-keyaxis/series.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prefix.html">
    <span class="label">Previous</span>
    <span class="title">prefix</span>
  </a>
  <a class="next-link" href="suffix.html">
    <span class="label">Next</span>
    <span class="title">suffix</span>
  </a>
</div>
