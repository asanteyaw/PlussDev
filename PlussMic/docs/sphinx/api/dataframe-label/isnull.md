<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37cf2ec2178f -->

# DataFrameLabelAccessor::isnull

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
Shelf DataFrameLabelAccessor::isnull() const
```

## What It Does

Marks each row `true` where its case label is null, `false` where it's a
genuine value.

## Returns

`Shelf`
    A boolean shelf, one entry per row, `display_name` `"isnull"`, displayed
    against the case labels themselves.

## Examples

Example 1. A numeric case-label axis with one null.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Shelf(pluss::make_series({1.0, 2.0, 3.0}))}},
    Shelf(pluss::make_series({10.0, 0.0, 30.0}), pluss::make_series(std::vector<bool>{true, false, true})));

auto out = readings.labels().isnull();
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

- Works regardless of whether the case labels are structured or a flat/default
  axis — it always inspects the flat displayed case-label shelf.

## See Also

- [`DataFrameLabelAccessor::notnull`](notnull.md): the exact logical negation of
  this method.
- [`DataFrameLabelAccessor::fillnull`](fillnull.md): replace nulls instead of
  just locating them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect.html">
    <span class="label">Previous</span>
    <span class="title">intersect</span>
  </a>
  <a class="next-link" href="levels.html">
    <span class="label">Next</span>
    <span class="title">levels</span>
  </a>
</div>
