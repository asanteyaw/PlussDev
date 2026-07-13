# DataFrame::mean_by

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mean_by(std::string_view group_feature, std::string_view value_feature, std::int64_t top_n = 0, std::int64_t min_count = 1, bool dropna_group = true, bool render = false) -> DataFrameView
```

## What It Does

Groups by one feature and returns the mean of another feature within each group.

## Parameters

**group_feature**
    Feature whose distinct values define the groups for grouped summaries such as `mean_by(...)`.

**value_feature**
    Feature whose values populate pivot cells or grouped summaries.

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

**dropna_group**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

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

auto out = df.mean_by("city", "sales", 3, 1, true, false);
```

### Output

```text
city    sales_mean
Accra   110.0
Tema    120.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
