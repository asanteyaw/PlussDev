# DataFrame::drop

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::drop(pluss::RangeView<std::string_view> names) -> DataFrame
```

Remove one or more features and keep the remaining frame unchanged.

`drop(...)` is a schema operation. It never removes rows.

Use it when your pipeline should keep the same cases but carry forward a narrower feature set.

## Parameters

**names**

Feature names to remove.

Pass one or more names in any order. Every remaining feature keeps its original left-to-right order.

This is therefore the complement of `features({...})`: instead of naming what to keep, you name what to remove.

## Returns

`DataFrame`
    New frame containing every unselected feature in its original row alignment.

## Notes

`drop(...)` works on feature names, not on case labels.

The current case axis is preserved exactly. Only the feature set changes.

The surviving features keep their original left-to-right order; `drop(...)` does not re-sort the schema.

## Examples

Example 1. Drop one feature and preserve the case axis.

```cpp
using namespace pluss::table;

auto labeled = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).caselbl(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.drop({"region"});
```

### Output

```text
===========================
     | city    units  price
---------------------------
A100 | Accra   10     3
B200 | Tema    7      4
B200 | Kumasi  12     2.5
D400 | Tamale  5      5
---------------------------
[4 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="drop_duplicates.html">
    <span class="label">Next</span>
    <span class="title">drop_duplicates</span>
  </a>
</div>
