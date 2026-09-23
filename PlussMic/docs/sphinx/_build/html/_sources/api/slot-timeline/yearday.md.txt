<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7e105342c7e5 -->

# SlotTimelineAccessor::yearday

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::yearday() const
```

## What It Does

Extracts the day-of-year (`1`-`366`) of each value in the bound slot,
replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of day-of-year numbers.

## Examples

Example 1. Extract the day-of-year from a late-January and a
late-February timestamp.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().yearday();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
31 60
```

`"2024-01-31"` is the 31st day of the year; `"2024-02-29"` is the 60th —
the extra leap day pushes every date from March onward one day later
than it would fall in a non-leap year.

## Notes

- `1` is January 1st; the maximum is `366` in a leap year, `365`
  otherwise — see [`leap()`](leap.md).
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::day`](day.md): the day-of-*month* number
  instead of day-of-year.
- [`SlotTimelineAccessor::week`](week.md): the ISO week-of-year, a
  coarser-grained position within the year.
- [`SlotTimelineAccessor::leap`](leap.md): whether the value's year has
  the extra day that shifts this number.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
