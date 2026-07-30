# Atlas::map_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::map_frames(const std::function<DataFrame(const DataFrame&)>& fn) -> Atlas
```

## What It Does

Applies the same function to each frame and returns the transformed atlas.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map_frames([](const pluss::table::DataFrame& frame) { return frame.head(1, false); });
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
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="ncases.html">
    <span class="label">Next</span>
    <span class="title">ncases</span>
  </a>
</div>
