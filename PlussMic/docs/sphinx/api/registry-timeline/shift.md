# RegistryTimelineAccessor::shift

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
RegistryTimelineAccessor::shift(std::int64_t periods, std::string_view freq) -> Registry
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**periods**
    Number of row or time steps to shift forward or backward. Positive values usually move data down or later; negative values move it up or earlier.

**freq**
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto timelines = rg.fields({"ts"}).tl();

auto out = timelines.shift(1, "2D");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html">
    <span class="label">Previous</span>
    <span class="title">second</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
