# AxelTimelineAccessor::period

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
AxelTimelineAccessor::period(std::string_view freq) -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**freq**
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-01T09:40:00Z")})},
});
auto r1 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-02T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto timelines = axel.fields({"ts"}).tl();

auto out = timelines.period("2D");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_start.html">
    <span class="label">Previous</span>
    <span class="title">month_start</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
