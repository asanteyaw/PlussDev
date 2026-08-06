<!-- pluss-hand-authored -->

# SlotTimelineAccessor::shift

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const
```

## What It Does

Shifts each value in the bound slot forward (or backward, if `periods`
is negative) by `periods * freq`, replacing each `Timeline` value's
values in place. Month/year shifts are calendar-aware: a day-of-month
that doesn't exist in the target month is clamped down to that month's
last valid day.

## Parameters

**periods** (`std::int64_t`)
    How many `freq` units to shift by. Positive moves forward in time,
    negative moves backward.

**freq** (`std::string_view`)
    One of `"y"`/`"yr"`/`"year"`/`"years"`/`"annual"`,
    `"mo"`/`"mth"`/`"month"`/`"months"`, `"d"`/`"day"`/`"days"`,
    `"h"`/`"hr"`/`"hour"`/`"hours"`, `"t"`/`"min"`/`"minute"`/`"minutes"`, or
    `"s"`/`"sec"`/`"second"`/`"seconds"` (case-insensitive). Cannot be empty.

## Returns

`Slot`
    A new slot with each value shifted by `periods * freq`.

## Examples

Example 1. Shift two timestamps forward one calendar month, showing
day-of-month clamping on the January value.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().shift(1, "months");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-02-29 09:15:30 | 2024-03-29 23:45:00
```

January 31st shifted one month forward lands on February 29th (2024 is a
leap year) — day `31` is clamped down to February's last valid day rather
than overflowing into March.

Example 2. An unrecognized frequency throws.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
try {
  auto out = slot.tl().shift(1, "bogus");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Timeline::shift: unsupported frequency
```

## Notes

- `freq` accepts calendar units (`years`/`months`, day-of-month-clamping)
  and fixed units (`days`/`hours`/`minutes`/`seconds`, exact elapsed-time
  arithmetic with no clamping) — there's no `"week"` frequency.
- `freq` cannot be empty, and an unrecognized string throws, per Example 2.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::period`](period.md): group timestamps into
  fixed periods rather than shifting each one.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html">
    <span class="label">Previous</span>
    <span class="title">second</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
