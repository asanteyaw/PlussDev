# DataFrame::lengthen

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::lengthen(pluss::RangeView<std::string_view> id_features = {}, pluss::RangeView<std::string_view> value_features = {}, std::string feature_name = "feature", std::string value_name = "value", bool fresh_cases = true) -> DataFrame
```

## What It Does

Reshapes a wide frame into long form by tiling identifier features and stacking homogeneous value features.

## Parameters

**id_features**
    Features repeated unchanged for each selected value feature.

**value_features**
    Homogeneous features stacked into the output value feature. Leave empty to use every non-identifier feature.

**feature_name**
    Output name for the Note-backed feature that identifies each source value feature.

**value_name**
    Output name for the stacked typed values.

**fresh_cases**
    When `true`, create contiguous integer cases; when `false`, repeat the original case axis for every value feature.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto wide = pluss::table::DataFrame({
    {"id", pluss::make_note({"r1", "r2"})},
    {"a", pluss::make_series({1.0, 2.0})},
    {"b", pluss::make_series({3.0, 4.0})},
});
auto out = wide.lengthen({"id"}, {"a", "b"});
```

### Output

```text
id  feature  value
r1  a        1.0
r2  a        2.0
r1  b        3.0
r2  b        4.0
```

## Reshape Rules

- `lengthen(...)` tiles identifier features and stacks each value feature as one contiguous case block.
- Value features must share a Thread kind; numeric value features must also share dtype and platform.
- Payload concatenation and identifier tiling use Series/Thread bulk operations rather than cell-wise conversion.
- Missing-value masks are concatenated with the payload and preserved.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="level.html">
    <span class="label">Next</span>
    <span class="title">level</span>
  </a>
</div>
