# DataFramePlot3DAccessor::hist

[Back to DataFramePlot3DAccessor API Reference](../../dataframe_plot3d_accessor_api_reference)

## Signature

```cpp
DataFramePlot3DAccessor::hist(std::string_view x_feature, std::string_view y_feature, const Hist3DPlotOptions& options = {}) -> pluss::pl::Studio
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**y_feature**
    Single numeric feature to draw on the vertical axis or as the primary plotted measure.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`pluss::pl::Studio`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"x", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"y", pluss::table::Thread(pluss::make_series({4.0, 5.0, 6.0}))},
    {"z", pluss::table::Thread(pluss::make_series({7.0, 8.0, 9.0}))},
});
auto plot3d = df.plot3d();

auto out = plot3d.hist("units", "units", {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="line.html">
    <span class="label">Next</span>
    <span class="title">line</span>
  </a>
</div>
