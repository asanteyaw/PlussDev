<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dfe4206f3881 -->

# Archive::ffill

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::ffill() const
```

## What It Does

Forward-fills `NULL` cells across every catalog in the archive — each `NULL` cell takes the
value of the nearest preceding non-`NULL` cell in the same stream, per catalog. Delegates to
each catalog's own [`Catalog::ffill()`](../catalog/ffill.md); a leading `NULL` with no
preceding value stays `NULL`.

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with `NULL` cells
    forward-filled in every catalog.

## Examples

Example 1. Forward-fill a masked `NULL`.

```cpp
using namespace pluss::kindle::literals;

auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.ffill();
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
id                       | Series       | Series[float64]  | 3       | [1, 1, 3]                                        
========================================================================================================================
```

## Notes

- A leading `NULL` with no preceding value in its own catalog stays `NULL`.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Depth labels are preserved on the result.

## See Also

- [`Archive::bfill`](bfill.md): the mirror — fills from the next value instead of the
  previous one.
- [`Archive::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.
- [`Catalog::ffill`](../catalog/ffill.md): the single-catalog delegate this method applies
  to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
