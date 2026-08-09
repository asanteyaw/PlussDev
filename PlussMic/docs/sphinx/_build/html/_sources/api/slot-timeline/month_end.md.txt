<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0c2e5aa8c938 -->

# SlotTimelineAccessor::month_end

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::month_end() const
```

## What It Does

Flags whether each value in the bound slot falls on the last day of its
calendar month, replacing each `Timeline` value with a `Bool` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` where the value's day-of-month is
    that month's last valid day, `false` otherwise.

## Examples

Example 1. Both sample timestamps are the last day of their respective
month.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().month_end();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

`"2024-01-31"` is January's last day; `"2024-02-29"` is February's last
day in this leap year — see [`leap()`](leap.md).

## Notes

- Correctly accounts for each month's actual length, including
  leap-year February.
- Only checks the date component; time-of-day doesn't affect the
  result.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month_start`](month_start.md): the mirror
  check at the beginning of the month.
- [`SlotTimelineAccessor::leap`](leap.md): where leap-year status
  actually changes February's last valid day.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
