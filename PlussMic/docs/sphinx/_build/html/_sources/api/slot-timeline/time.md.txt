<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e4a694e1d83b -->

# SlotTimelineAccessor::time

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::time() const
```

## What It Does

Keeps only the time-of-day portion of each value in the bound slot — the
date is reset to the Unix epoch day (`1970-01-01`) — replacing each
`Timeline` value's values in place.

## Parameters

`None`

## Returns

`Slot`
    A new slot with each value's date portion reset to `1970-01-01`,
    keeping only the time-of-day.

## Examples

Example 1. Keep only the time-of-day from two timestamps.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().time();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
1970-01-01 09:15:30 | 1970-01-01 23:45:00
```

Every value's date is now `1970-01-01`; each original time-of-day is
preserved exactly.

## Notes

- The result is still `Timeline`-kind with a fixed date component — it's
  meant for comparing or grouping by time-of-day, not as a display format.
  Use [`fmt("%H:%M:%S")`](fmt.md) for a bare text time.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::date`](date.md): the mirror-image operation,
  keeping the date and zeroing the time-of-day.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
