<!-- pluss-hand-authored -->

# SlotTimelineAccessor::day

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::day() const
```

## What It Does

Extracts the day-of-month (`1`-`31`) of each value in the bound slot,
replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of day-of-month numbers.

## Examples

Example 1. Extract the day-of-month from two end-of-month timestamps.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().day();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
31 29
```

`2024-02-29` is valid — 2024 is a leap year, so February has 29 days that
year; see [`leap()`](leap.md).

## Notes

- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::year`](year.md) / [`month()`](month.md): the
  other calendar-date components.
- [`SlotTimelineAccessor::yearday`](yearday.md): the day-of-*year* number
  instead of day-of-month.
- [`SlotTimelineAccessor::month_end`](month_end.md): whether this day is
  the last day of its month.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
