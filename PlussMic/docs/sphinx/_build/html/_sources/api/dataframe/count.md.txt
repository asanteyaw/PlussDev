# DataFrame::count

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::count(Axis axis = Axis::Case) -> Thread
```

Count non-missing values along the requested axis.

`DataFrame::count(...)` can summarize by feature or by case depending on the axis you choose.

## Parameters

**axis**

Reduction direction.

- `Axis::Feature`: one count per feature.
- `Axis::Case`: one count per row across the currently selected features.

## Returns

`Thread`
    Count result whose labels follow the retained axis.

## Notes

`Axis::Feature` counts down each feature and returns one count per feature.

`Axis::Case` counts across the selected features for each row and returns one count per case.

## Examples

Example 1. Count valid values per feature.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.count(Axis::Feature);
```

### Output

```text
x  4
   y  4
   z  4
city  4
dtype: Series
```

Example 2. Count valid values across selected features for each row.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.features({"x", "y", "z"}).count(Axis::Case);
```

### Output

```text
0  3
1  3
2  3
3  3
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr_with.html">
    <span class="label">Previous</span>
    <span class="title">corr_with</span>
  </a>
  <a class="next-link" href="count_by.html">
    <span class="label">Next</span>
    <span class="title">count_by</span>
  </a>
</div>
