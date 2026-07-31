# SlotTimelineAccessor::year

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
SlotTimelineAccessor::year() -> Slot
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

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

auto out = timelines.year();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
