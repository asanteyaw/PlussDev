<!-- pluss-hand-authored -->

# SlotTimelineAccessor::date

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::date() const
```

## What It Does

Strips the time-of-day from each value in the bound slot (dropped to
midnight), replacing each `Timeline` value's values in place. Equivalent
to [`floor("D")`](floor.md).

## Parameters

`None`

## Returns

`Slot`
    A new slot with each value's time-of-day portion zeroed out.

## Examples

Example 1. Strip the time-of-day from two timestamps.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().date();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 00:00:00 | 2024-02-29 00:00:00
```

## Notes

- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`SlotTimelineAccessor::floor`](floor.md): the general form `date()`
  is a fixed `"D"` unit of.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
