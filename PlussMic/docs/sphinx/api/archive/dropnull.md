<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc2e4f518682 -->

# Archive::dropnull

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::dropnull() const
```

## What It Does

Removes rows with any `NULL` cell from every catalog in the archive — delegating to each
catalog's own [`Catalog::dropnull()`](../catalog/dropnull.md) independently. Because it
runs per-catalog, a ragged archive's catalogs can end up with different record counts after
this call (each catalog may lose a different number of rows).

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with `NULL`-containing rows
    removed from every catalog independently.

## Examples

Example 1. Drop the row containing a `NULL`.

```cpp
using namespace pluss::kindle::literals;

auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.dropnull();
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
id                       | Series       | Series[float64]  | 2       | [1, 3]                                           
city                     | Note         | Note             | 2       | [Accra, Kumasi]                                  
========================================================================================================================
```

## Notes

- Runs per-catalog independently — catalogs with different `NULL` patterns can end up with
  different record counts, making the archive newly ragged even if it wasn't before.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Depth labels are preserved on the result.

## See Also

- [`Archive::fillnull`](fillnull.md) / [`Archive::ffill`](ffill.md) /
  [`Archive::bfill`](bfill.md): fill `NULL` cells instead of removing their rows.
- [`Catalog::dropnull`](../catalog/dropnull.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
