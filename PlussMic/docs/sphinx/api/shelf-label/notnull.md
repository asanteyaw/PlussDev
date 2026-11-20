<!-- pluss-hand-authored -->

# ShelfLabelAccessor::notnull

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::notnull() const
```

## What It Does

Marks each row `true` where its case label is a genuine (non-null) value,
`false` where it's null — the exact logical negation of
[`isnull()`](isnull.md).

## Parameters

`None`

## Returns

`Shelf`
    A `Bool` shelf, one entry per row, `display_name` `"notnull"`, displayed
    against the case labels themselves.

## Examples

Example 1. A numeric case-label axis with one null.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Shelf(pluss::make_series(
        {10.0, 0.0, 30.0}), pluss::make_series(std::vector<bool>{true, false, true})));

auto out = readings.label().notnull();
std::cout << out;
```

### Output

```text
  10  true
NULL  false
  30  true
Name: notnull, ptype: Series
```

## Notes

- Inspects the case labels, not the shelf's own data.
- The exact logical negation of [`isnull()`](isnull.md).

## See Also

- [`ShelfLabelAccessor::isnull`](isnull.md): the exact logical negation of
  this method.
- [`ShelfLabelAccessor::fillnull`](fillnull.md): replace nulls instead of
  just locating them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="monotonic.html">
    <span class="label">Previous</span>
    <span class="title">monotonic</span>
  </a>
  <a class="next-link" href="pos.html">
    <span class="label">Next</span>
    <span class="title">pos</span>
  </a>
</div>
