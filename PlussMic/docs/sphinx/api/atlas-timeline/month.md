# AtlasTimelineAccessor::month

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::month() -> Atlas
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto q1 = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))},
});
auto q2 = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-02T09:10:00Z", "2024-01-02T09:40:00Z"}))},
});
auto atlas = pluss::table::Atlas({q1, q2});
auto timelines = atlas.tl();

auto out = timelines.month();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html">
    <span class="label">Previous</span>
    <span class="title">minute</span>
  </a>
  <a class="next-link" href="month_end.html">
    <span class="label">Next</span>
    <span class="title">month_end</span>
  </a>
</div>
