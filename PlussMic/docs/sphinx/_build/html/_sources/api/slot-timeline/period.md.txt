<!-- pluss-hand-authored -->

# SlotTimelineAccessor::period

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::period(std::string_view freq) const
```

## What It Does

Replaces each value in the bound slot with the *start* of the
`freq`-sized period it falls within, replacing each `Timeline` value's
values in place. For the fixed clock units, this is equivalent to
[`floor(freq)`](floor.md); for the calendar units it accepts, it snaps
to the start of that calendar period (e.g. the first of the month).

## Parameters

**freq** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported frequency throws.

## Returns

`Slot`
    A new slot with each value replaced by its period start.

## Examples

Example 1. Snap a mid-day timestamp to its containing day's start.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
auto out = slot.tl().period("D");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0);
```

### Output

```text
2024-01-31 00:00:00
```

## Notes

- For the fixed units this method accepts, `period(freq)` behaves the same
  as [`floor(freq)`](floor.md) — see that page for the unrecognized-unit
  throw text.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::floor`](floor.md): the underlying rounding
  behavior this method shares for fixed units.
- [`SlotTimelineAccessor::shift`](shift.md): move each timestamp by a
  fixed offset instead of snapping to a period boundary.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

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
