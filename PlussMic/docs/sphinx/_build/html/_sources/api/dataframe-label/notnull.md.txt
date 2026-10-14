<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8007230a14aa -->

# DataFrameLabelAccessor::notnull

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::notnull() const -> Slate
```

## What It Does

Marks each row `true` where its case label is a genuine (non-null) value,
`false` where it's null — the exact logical negation of
[`isnull()`](isnull.md).

## Returns

`Slate`
    A boolean slate, one entry per row, `display_name` `"notnull"`, displayed
    against the case labels themselves.

## Examples

Example 1. A numeric case-label axis with one null.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0, 3.0}))}},
    Slate(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0})));

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

- Works regardless of whether the case labels are structured or a flat/default
  axis — it always inspects the flat displayed case-label slate.

## See Also

- [`DataFrameLabelAccessor::isnull`](isnull.md): the exact logical negation of
  this method.
- [`DataFrameLabelAccessor::fillnull`](fillnull.md): replace nulls instead of
  just locating them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="names.html">
    <span class="label">Previous</span>
    <span class="title">names</span>
  </a>
  <a class="next-link" href="order.html">
    <span class="label">Next</span>
    <span class="title">order</span>
  </a>
</div>
