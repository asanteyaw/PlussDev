<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c2b3a9926b7e -->

# SlotTimelineAccessor::second

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::second() const
```

## What It Does

Extracts the second-of-minute (`0`-`59`) of each value in the bound
slot, replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of second numbers.

## Examples

Example 1. Extract the second component from two timestamps.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().second();
std::cout << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
30 0
```

## Notes

- Truncates any sub-second precision — reports whole seconds only.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::hour`](hour.md) / [`minute()`](minute.md): the
  neighboring time-of-day components.
- [`SlotTimelineAccessor::time`](time.md): the whole time-of-day portion
  at once.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
