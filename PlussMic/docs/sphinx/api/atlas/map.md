# Atlas::map

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::map(const std::function<ThreadValue(const ThreadValue&)>& fn) -> Atlas
```

## What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="loc.html">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="map_frames.html">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>
