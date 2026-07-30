# AtlasTimelineAccessor::round

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::round(std::string_view unit) -> Atlas
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**unit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto q1 = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))},
});
auto q2 = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-02T09:10:00Z", "2024-01-02T09:40:00Z"}))},
});
auto atlas = pluss::table::Atlas({q1, q2});
auto timelines = atlas.tl();

auto out = timelines.round("D");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
