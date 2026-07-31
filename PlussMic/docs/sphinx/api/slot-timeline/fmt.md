# SlotTimelineAccessor::fmt

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
SlotTimelineAccessor::fmt(std::string_view pattern) -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")});
auto timelines = slot.tl();

auto out = timelines.fmt("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
