# Atlas::plot

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::plot() -> AtlasPlotAccessor

Atlas::plot(const AtlasPlotOptions& options) -> AtlasPlotAccessor
```

## What It Does

This opens the plotting accessor for the current data object.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`AtlasPlotAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto fig = atlas.plot(0).line("sales");
```

### Output

```text
Figure(line)
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto fig = atlas.plot(0).line("sales");
```

### Output

```text
Figure(line)
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfeatures.html">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="relabel_depth.html">
    <span class="label">Next</span>
    <span class="title">relabel_depth</span>
  </a>
</div>
