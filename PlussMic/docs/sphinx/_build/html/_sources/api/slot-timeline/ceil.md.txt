<!-- pluss-hand-authored -->

# SlotTimelineAccessor::ceil

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds each value in the bound slot *up* to the nearest `unit`
boundary, replacing each `Timeline` value's values in place (stays
`Timeline`-kind).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Slot`
    A new slot with each value ceiled to `unit`.

## Examples

Example 1. Ceil two timestamps to the nearest hour.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().ceil("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 10:00:00 | 2024-03-01 00:00:00
```

Both values advance to the start of their *next* hour — `09:15:30` to
`10:00:00`, and `23:45:00` rolling over the day boundary into
`2024-03-01 00:00:00`.

## Notes

- A value already exactly on a `unit` boundary is left unchanged, not
  advanced to the next boundary.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws
  `"Timeline: unsupported unit (expected S/min/H/D)"`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::floor`](floor.md) / [`round()`](round.md):
  round down, or to the nearest boundary, instead of always up.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
