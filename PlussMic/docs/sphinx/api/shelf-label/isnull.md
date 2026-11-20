<!-- pluss-hand-authored -->

# ShelfLabelAccessor::isnull

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::isnull() const
```

## What It Does

Marks each row `true` where its case label is null, `false` where it's a
genuine value.

## Parameters

`None`

## Returns

`Shelf`
    A `Bool` shelf, one entry per row, `display_name` `"isnull"`, displayed
    against the case labels themselves.

## Examples

Example 1. A numeric case-label axis with one null.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Shelf(pluss::make_series(
        {10.0, 0.0, 30.0}), pluss::make_series(std::vector<bool>{true, false, true})));

auto out = readings.label().isnull();
std::cout << out;
```

### Output

```text
  10  false
NULL  true
  30  false
Name: isnull, ptype: Series
```

## Notes

- Inspects the case labels, not the shelf's own data.

## See Also

- [`ShelfLabelAccessor::notnull`](notnull.md): the exact logical negation of
  this method.
- [`ShelfLabelAccessor::fillnull`](fillnull.md): replace nulls instead of
  just locating them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect.html">
    <span class="label">Previous</span>
    <span class="title">intersect</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
