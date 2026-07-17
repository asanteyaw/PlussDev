# DataFrame::pivot_table

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::pivot_table(std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, GroupOp op = GroupOp::Mean, bool dropnull_keys = true, bool render = false) -> DataFrameView
```

## What It Does

Builds a pivoted summary table using an aggregation function when multiple rows map to the same cell.

## Parameters

**case_feature**
    Feature whose distinct values become the output row groups in the pivoted summary result.

**feature_feature**
    Feature whose distinct values become new output columns in pivot-style reshaping.

**value_feature**
    Feature whose values populate pivot cells or grouped summaries.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

**dropnull_keys**
    When `true`, rows with null pivot/group keys are excluded before reshaping.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.pivot_table("city", "product", "sales", pluss::table::DataFrame::GroupOp::Mean, true, false);
```

### Output

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pivot.html">
    <span class="label">Previous</span>
    <span class="title">pivot</span>
  </a>
  <a class="next-link" href="place.html">
    <span class="label">Next</span>
    <span class="title">place</span>
  </a>
</div>
