<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fbc521c168ba -->

# StreamTimelineAccessor::shift

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const
```

## What It Does

Shifts the bound stream's timestamp forward (or backward, if `periods` is
negative) by `periods * freq`, replacing its values in place. Month/year
shifts are calendar-aware: a day-of-month that doesn't exist in the target
month is clamped down to that month's last valid day.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

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

`Stream`
    A new stream with its timestamp shifted by `periods * freq`.

## Examples

Example 1. Shift two timestamps forward one calendar month, showing
day-of-month clamping on the January value.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().shift(1, "months");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
2024-02-29 09:15:30 | 2024-03-29 23:45:00
```

January 31st shifted one month forward lands on February 29th (2024 is a
leap year) — day `31` is clamped down to February's last valid day rather
than overflowing into March.

Example 2 (edge case). An unrecognized frequency throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
try {
  auto out = stream.tl().shift(1, "bogus");
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

- `freq` accepts calendar units (`years`/`months`, day-of-month-clamping) and
  fixed units (`days`/`hours`/`minutes`/`seconds`, exact elapsed-time
  arithmetic with no clamping) — there's no `"week"` frequency.
- `freq` cannot be empty, and an unrecognized string throws, per Example 2.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::period`](period.md): group timestamps into fixed
  periods rather than shifting each one.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::shift`](../catalog-timeline/shift.md): the
  whole-catalog equivalent.

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
