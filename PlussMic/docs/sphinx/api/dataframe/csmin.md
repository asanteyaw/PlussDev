# DataFrame::csmin

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::csmin(Axis axis = Axis::Case, bool skipna = true) -> Thread
```

Return the coordinate of the minimum value along the requested axis.

`csmin(...)` is the argmin-style counterpart to `csmax(...)`.

## Parameters

**axis**

Direction in which to search for the minimum coordinate.

**skipna**

Missing-value policy during the search.

## Returns

`Thread`
    Label/position thread describing where each minimum was found.

## Notes

With `Axis::Feature`, the output identifies which case index or case label holds the minimum for each selected feature.

## Examples

Example 1. For each selected feature, report which case holds the minimum value.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
});

auto out = corr_base.features({"x", "z"}).csmin(Axis::Feature);
```

### Output

```text
x  0
z  1
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="csmax.html">
    <span class="label">Previous</span>
    <span class="title">csmax</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
