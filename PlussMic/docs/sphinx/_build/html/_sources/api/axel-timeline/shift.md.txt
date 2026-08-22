<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 830e8779fa90 -->

# AxelTimelineAccessor::shift

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const
```

## What It Does

Shifts each bound field's timestamp forward (or backward, if `periods`
is negative) by `periods * freq`, in every registry frame, replacing
each `Timeline` field's values in place. Month/year shifts are
calendar-aware: a day-of-month that doesn't exist in the target month
is clamped down to that month's last valid day. Applies
[`RegistryTimelineAccessor::shift`](../registry-timeline/shift.md) to
every frame via [`Axel::map`](../axel/map.md).

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

`Axel`
    A new axel with every bound field's timestamps, in every frame,
    shifted by `periods * freq`; every unbound field unchanged.

## Examples

Example 1. Shift two timestamps forward one calendar month, showing
day-of-month clamping on the January value.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().shift(1, "months");
auto fmted = out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(1)).at(0);
```

### Output

```text
2024-02-29 09:15:30 | 2024-03-29 23:45:00
```

January 31st shifted one month forward lands on February 29th (2024 is
a leap year) — day `31` is clamped down to February's last valid day
rather than overflowing into March.

Example 2. An unrecognized frequency throws.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::scribble::Axel({r0});

try {
  auto out = axel.fields({"ts"}).tl().shift(1, "bogus");
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
  and fixed units (`days`/`hours`/`minutes`/`seconds`, exact
  elapsed-time arithmetic with no clamping) — there's no `"week"`
  frequency.
- `freq` cannot be empty, and an unrecognized string throws, per
  Example 2.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::period`](period.md): group timestamps into
  fixed periods rather than shifting each one.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
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
