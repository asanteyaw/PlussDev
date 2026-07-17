# DataFrame::csmax

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::csmax(Axis axis = Axis::Case, bool skipnull = true) -> Thread
```

Return the coordinate of the maximum value along the requested axis.

`csmax(...)` is the argmax-style surface: it tells you where the maximum occurred, not just what the maximum value was.

## Parameters

**axis**

Direction in which to search for the maximum coordinate.

- `Axis::Case`: argmax across selected features for each row.
- `Axis::Feature`: argmax across cases for each selected feature.

**skipnull**

Missing-value policy.

- `true`: skip missing values while searching.
- `false`: let missing state participate in the result policy.

## Returns

`Thread`
    Label/position thread describing where each maximum was found.

## Notes

With `Axis::Case`, each output row tells you which selected feature won for that case.

With `Axis::Feature`, each output feature tells you which case achieved the maximum for that feature.

## Examples

Example 1. For each row, report which selected feature holds the maximum value.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
});

auto out = corr_base.features({"x", "z"}).csmax(Axis::Case);
```

### Output

```text
0  z
1  x
2  x
3  x
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="covar.html">
    <span class="label">Previous</span>
    <span class="title">covar</span>
  </a>
  <a class="next-link" href="csmin.html">
    <span class="label">Next</span>
    <span class="title">csmin</span>
  </a>
</div>
