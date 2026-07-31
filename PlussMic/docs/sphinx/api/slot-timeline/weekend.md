# SlotTimelineAccessor::weekend

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
SlotTimelineAccessor::weekend() -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")});
auto timelines = slot.tl();

auto out = timelines.weekend();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
