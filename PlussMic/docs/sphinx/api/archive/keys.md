<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e2bdd65bc8af -->

# Archive::keys

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::keys(std::span<const std::string_view> keys) const -> ArchiveKeyAccessor
Archive::keys(std::initializer_list<std::string_view> keys) const -> ArchiveKeyAccessor
```

## What It Does

Selects one or more keys (present in each catalog, uniformly) and returns an
`ArchiveKeyAccessor` — an entry point into a family of operations
(`fillnull`, `dropnull`, `ffill`, `bfill`, `isnull`, `notnull`, `invalid`, `distinct`,
`freq`, `ndistinct`, `astype`, plus a `cast()` sub-accessor) applied to just the selected
streams *within every catalog*, returning a new `Archive` scoped to that selection.

```{important} No no-argument overload — unlike Catalog::keys()
[`Catalog::keys()`](../catalog/keys.md) has a no-argument overload returning
`const std::vector<std::string>&` (every stream name). `Archive::keys()` has **no**
equivalent — every overload requires an explicit name list. To list a single catalog's own
key names, go through [`catalog(i)`](catalog.md)`.keys()` instead.
```

```{important} keys(names) is a selection, not a rename or a projection to a smaller archive
`archive.keys("id").fillnull(0.0)` operates on the `id` stream *within each catalog's own
context* and returns an `Archive` containing only the selected streams, transformed, in
every catalog. If you want to keep every stream but only transform some, transform the
full archive directly.
```

## Parameters

**keys** (`std::span<const std::string_view>` or `std::initializer_list<std::string_view>`)
    One or more stream names to select, applied to every catalog in the archive.

## Returns

`ArchiveKeyAccessor`
    An accessor scoped to the selected stream(s) — call one of its methods to actually
    transform them.

## Examples

Example 1. Select a stream by name and fill its `NULL` cells, scoped to that stream alone
in every catalog.

```cpp
using namespace pluss::kindle::literals;

auto mask0 = pluss::make_series(std::vector<bool>{true, false});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}), mask0),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto mask1 = pluss::make_series(std::vector<bool>{true, false});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0}), mask1),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Sunyani"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.keys({"id"}).fillnull(0.0);
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 0]                                           
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

`city`'s own `NULL`-free content is unaffected, in both catalogs — only `id` (the selected
stream) was transformed.

## Notes

- No no-argument overload — see the first important callout above.
- The full `ArchiveKeyAccessor` surface
  (`fillnull`/`dropnull`/`ffill`/`bfill`/`isnull`/`notnull`/`invalid`/`astype`/`distinct`/
  `freq`/`ndistinct`/`cast()`) is out of scope for this page — see
  [`Catalog::keys`](../catalog/keys.md) for the same shape of accessor on a single catalog;
  every method there has an identical counterpart here, scoped to every catalog in the
  archive instead of one.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::has`](has.md): check whether a key exists before selecting it.
- [`Archive::select`](select.md) / [`Archive::drop`](drop.md): keep or remove whole streams
  from every catalog, rather than scoping a transform to a selection.
- [`Catalog::keys`](../catalog/keys.md): the single-catalog equivalent, including its
  no-argument overload this method has no counterpart for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keyaxis.html">
    <span class="label">Previous</span>
    <span class="title">keyaxis</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
