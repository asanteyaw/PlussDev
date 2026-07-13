# DataFrame::plot3d

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::plot3d() -> DataFramePlot3DAccessor

DataFrame::plot3d(const DataFramePlotOptions& options) -> DataFramePlot3DAccessor
```

## What It Does

Opens the 3D plotting accessor for the current frame.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFramePlot3DAccessor`
    Value produced by this call.

## Examples

Example 1. Create the 3D plotting accessor with default plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0})},
    {"y", pluss::make_series({4.0, 3.0, 2.0})},
    {"z", pluss::make_series({2.0, 5.0, 4.0})},
});

auto fig = df.plot3d().scatter("x", "y", "z");
```

### Output

```text
Figure(scatter3d/line3d)
```
Example 2. Create the 3D plotting accessor with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0})},
    {"y", pluss::make_series({4.0, 3.0, 2.0})},
    {"z", pluss::make_series({2.0, 5.0, 4.0})},
});

pluss::table::DataFramePlotOptions options;
options.width = 900.0;
auto fig = df.plot3d(options).scatter("x", "y", "z");
```

### Output

```text
Figure(scatter3d/line3d)
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
