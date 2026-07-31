# SlotTimelineAccessor::period

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
SlotTimelineAccessor::period(std::string_view freq) -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**freq**
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")});
auto timelines = slot.tl();

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
