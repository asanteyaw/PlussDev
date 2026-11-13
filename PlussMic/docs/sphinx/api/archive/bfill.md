<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9158c1730aa2 -->

# Archive::bfill

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::bfill() const
```

## What It Does

Backward-fills `NULL` cells across every catalog in the archive — each `NULL` cell takes
the value of the nearest following non-`NULL` cell in the same stream, per catalog. The
mirror of [`ffill()`](ffill.md); delegates to each catalog's own
[`Catalog::bfill()`](../catalog/bfill.md). A trailing `NULL` with no following value stays
`NULL`.

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with `NULL` cells
    backward-filled in every catalog.

## Examples

Example 1. Backward-fill a masked `NULL`.

```cpp
using namespace pluss::kindle::literals;

auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.bfill();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 3, 3]                                        
========================================================================================================================
```

## Notes

- A trailing `NULL` with no following value in its own catalog stays `NULL`.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Depth labels are preserved on the result.

## See Also

- [`Archive::ffill`](ffill.md): the mirror — fills from the previous value instead of the
  next one.
- [`Archive::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.
- [`Catalog::bfill`](../catalog/bfill.md): the single-catalog delegate this method applies
  to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
