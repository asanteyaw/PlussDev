# Atlas::aggregate_over_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::aggregate_over_frames(const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) -> DataFrame
```

## What It Does

Reduces the full frame stack to one output by applying an aggregation function across the contained frames.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`DataFrame`
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

auto out = atlas.aggregate_over_frames([](const std::vector<pluss::table::DataFrame>& frames) { return frames.front(); });
```

### Output

```text
sales
10.0
20.0
```

## Practical Behavior

- `aggregate_over_frames(...)` hands the full frame vector to your reducer and expects one output object back.
- Use it when the aggregation depends on multiple frames together rather than frame-by-frame mapping.



<div class="pluss-inline-prev-next">
  <a class="next-link" href="align_depth.html">
    <span class="label">Next</span>
    <span class="title">align_depth</span>
  </a>
</div>
