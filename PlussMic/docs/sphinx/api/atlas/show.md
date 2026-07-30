# Atlas::show

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::show(const display::DisplayOptions& options = {}) -> void

Atlas::show(std::ostream& os, const display::DisplayOptions& options = {}) -> void
```

## What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**os**
    Destination output stream used instead of the default display or print target.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

atlas.show();
```

### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```
Example 2. Write the output to a caller-provided stream.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

atlas.show();
```

### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="relabel_depth.html">
    <span class="label">Previous</span>
    <span class="title">relabel_depth</span>
  </a>
  <a class="next-link" href="slice_frames.html">
    <span class="label">Next</span>
    <span class="title">slice_frames</span>
  </a>
</div>
