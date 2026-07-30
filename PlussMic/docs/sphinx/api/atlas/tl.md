# Atlas::tl

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::tl() -> AtlasTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`AtlasTimelineAccessor`
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

auto out = atlas.tl();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
