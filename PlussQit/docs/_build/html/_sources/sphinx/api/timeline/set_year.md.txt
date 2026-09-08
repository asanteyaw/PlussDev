# Timeline::set_year

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_year(int year) const
```

## Replace The Civil Year

`set_year()` creates a new `Timeline` by replacing the year shown in every
defined element's local calendar. It is a field replacement, not elapsed-time
arithmetic: month, day, clock fields, fractional nanoseconds, shape, element
order, and timezone remain unchanged.

That distinction decides when to use it. Use `set_year(2030)` when every value
must belong to calendar year 2030 while retaining its other fields. Use a
calendar offset when the instruction is instead "move each value forward by
some number of years."

Replacement is strict. A result must be a real local date for every defined
element. Replacing the year of `2024-02-29` with `2023` therefore throws; the
method does not silently choose 28 February. Absent instants remain absent and
do not participate in calendar validation.

## Example: Replace One Field, Preserve The Rest

```cpp
const auto t = pluss::timeline::make("2024-06-20T04:05:06.000000004Z");
const auto changed = t.set_year(2030);

auto options = pluss::timeline::FormatOptions{};
options.pattern = "%FT%T.%f%z";
std::cout << pluss::timeline::format(changed, options).at(0) << '\n';
```

### Output

```text
2030-06-20T04:05:06.000000004+00:00
```

The four trailing nanoseconds are intentionally visible: replacing a calendar
field does not reduce timestamp precision.

## Example: The Scalar Applies To Every Element

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});
const auto changed = t.set_year(2030);

std::cout << pluss::note::print(
    pluss::timeline::format(changed, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2030-01-15T23:58:59.123456789+00:00", "2030-06-20T04:05:06.000000004+00:00"], 
  ["2030-09-28T12:30:45.900000000+00:00", "2030-12-01T00:00:00.000000001+00:00"]
]
```

`year` is one scalar argument, not a per-element `Series`. The same year is
used at every logical position, including positions reached through a strided
view. The receiver is unchanged and the result owns its replacement values.

## Example: Invalid Leap-Day Replacement

```cpp
try {
  const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
  (void)leap.set_year(2023);
} catch (const pluss::Error&) {
  std::cout << "invalid calendar date rejected\n";
}
```

### Output

```text
invalid calendar date rejected
```

The operation is all-or-nothing from the caller's perspective: no partly
modified `Timeline` is returned if any defined element would become invalid.
The year argument itself is validated even when the receiver is empty.

## Timezones And Local Calendars

The replaced field is the year in `tz()`, not a year extracted from raw UTC
ticks. The timezone label is retained. Reconstructing the local date may select
a different UTC offset if that date belongs to a different daylight-saving
period. This is calendar editing, so preserving wall-clock fields takes
priority over preserving the original elapsed offset from UTC.

## See Also

- [`year`](year.md): inspect the local civil year.
- [`set_month`](set_month.md): replace the month without changing the year.
- [`tz_convert`](tz_convert.md): change the calendar view without changing instants.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_second.html"><span class="label">Previous</span><span class="title">set_second</span></a>
  <a class="next-link" href="shift.html"><span class="label">Next</span><span class="title">shift</span></a>
</div>
<!-- pluss-hand-authored -->
