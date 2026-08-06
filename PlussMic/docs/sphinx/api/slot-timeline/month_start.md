<!-- pluss-hand-authored -->

# SlotTimelineAccessor::month_start

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::month_start() const
```

## What It Does

Flags whether each value in the bound slot falls on the first day of
its calendar month, replacing each `Timeline` value with a `Bool`
`Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` where the value's day-of-month is
    `1`, `false` otherwise.

## Examples

Example 1. Neither sample timestamp falls on the 1st of its month.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().month_start();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
false false
```

Both sample dates are the *last* day of their month, not the first —
see [`month_end()`](month_end.md), which flags `true` for exactly these
values.

## Notes

- Only checks the day-of-month component (`day() == 1`); the
  time-of-day portion doesn't affect the result.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month_end`](month_end.md): the mirror check at
  the end of the month.
- [`SlotTimelineAccessor::day`](day.md): the underlying day-of-month
  number this method is derived from.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
