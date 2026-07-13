# DataFrame::plot

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::plot() -> DataFramePlotAccessor

DataFrame::plot(const DataFramePlotOptions& options) -> DataFramePlotAccessor
```

## What It Does

This opens the plotting accessor for the current data object.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFramePlotAccessor`
    Value produced by this call.

## Examples

Example 1. Create the plotting accessor with default plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto fig = df.plot().line("sales");
```

### Output

```text
Figure(line/scatter/bar/hist)
```
Example 2. Create the plotting accessor with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFramePlotOptions options;
options.default_x_feature = "id";
auto fig = df.plot(options).line("sales");
```

### Output

```text
Figure(line/scatter/bar/hist)
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="place.html">
    <span class="label">Previous</span>
    <span class="title">place</span>
  </a>
  <a class="next-link" href="plot3d.html">
    <span class="label">Next</span>
    <span class="title">plot3d</span>
  </a>
</div>
