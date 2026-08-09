<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 358b30d5b4a4 -->

# SlotTimelineAccessor::year

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::year() const
```

## What It Does

Extracts the calendar year of each value in the bound slot, replacing
each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of calendar years.

## Examples

Example 1. Extract the year from two timestamps in the same year.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().year();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
2024 2024
```

## Notes

- Reads local calendar fields relative to whatever timezone the value
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month`](month.md) / [`day()`](day.md): the
  other calendar-date components.
- [`SlotTimelineAccessor::leap`](leap.md): whether the extracted year is
  a leap year.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

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
