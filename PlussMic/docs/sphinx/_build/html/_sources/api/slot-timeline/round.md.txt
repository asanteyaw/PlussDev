<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5201c66d9656 -->

# SlotTimelineAccessor::round

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::round(std::string_view unit) const
```

## What It Does

Rounds each value in the bound slot to the *nearest* `unit` boundary,
replacing each `Timeline` value's values in place (stays
`Timeline`-kind).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Slot`
    A new slot with each value rounded to the nearest `unit`.

## Examples

Example 1. Round two timestamps to the nearest hour.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().round("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 09:00:00 | 2024-03-01 00:00:00
```

`09:15:30` is closer to `09:00:00` than `10:00:00`, so it rounds down;
`23:45:00` is closer to the next day's `00:00:00` than back to `23:00:00`,
so it rounds up and rolls into `2024-03-01`.

## Notes

- A value exactly halfway between two boundaries resolves **up**, never
  down.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws
  `"Timeline: unsupported unit (expected S/min/H/D)"`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::floor`](floor.md), [`ceil()`](ceil.md): always
  down, or always up, instead of nearest.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
