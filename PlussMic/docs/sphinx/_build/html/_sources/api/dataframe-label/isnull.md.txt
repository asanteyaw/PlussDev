<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::isnull

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::isnull() const -> Thread
```

## What It Does

Marks each row `true` where its case label is null, `false` where it's a
genuine value.

## Returns

`Thread`
    A boolean thread, one entry per row, `display_name` `"isnull"`, displayed
    against the case labels themselves.

## Examples

Example 1. A numeric case-label axis with one null.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({1.0, 2.0, 3.0}))}},
    Thread(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0})));

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

- Works regardless of whether the case labels are structured or a flat/default
  axis — it always inspects the flat displayed case-label thread.

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
