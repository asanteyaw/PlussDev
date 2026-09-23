<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2810afa482ba -->

# SlotTimelineAccessor::weekday

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::weekday() const
```

## What It Does

Extracts the weekday number (`0` = Monday through `6` = Sunday) of each
value in the bound slot, replacing each `Timeline` value with an `Int64`
`Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of weekday numbers.

## Examples

Example 1. Extract the weekday number from a Wednesday and a Thursday.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().weekday();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
2 3
```

`"2024-01-31"` is a Wednesday (`2`), `"2024-02-29"` is a Thursday (`3`) —
see [`weekday_name()`](weekday_name.md) for the same information spelled
out.

## Notes

- `0` is Monday, `6` is Sunday (ISO weekday numbering).
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a full English name.
- [`SlotTimelineAccessor::weekend`](weekend.md): whether the weekday
  falls on a weekend.
- [`SlotTimelineAccessor::week`](week.md): the ISO week-of-year this
  weekday belongs to.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
