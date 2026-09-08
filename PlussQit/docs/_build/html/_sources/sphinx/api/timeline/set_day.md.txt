# Timeline::set_day

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_day(int day) const
```

## Replace The Day Within Each Month

`set_day()` creates a new `Timeline` whose defined elements use `day` as their
local day-of-month. It retains each element's year, month, clock fields,
nanoseconds, and timezone. It also retains the receiver's rank and shape.

Use it for a calendar rule such as "place each timestamp on the first day of
its existing month." Do not use it to move forward or backward by a number of
days: replacement does not carry across month boundaries.

## Example: Normalize To The First Day

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});

std::cout << pluss::note::print(
    pluss::timeline::format(t.set_day(1), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-01T23:58:59.123456789+00:00", "2024-06-01T04:05:06.000000004+00:00"], 
  ["2024-09-01T12:30:45.900000000+00:00", "2024-12-01T00:00:00.000000001+00:00"]
]
```

Every element keeps its own year and month. Only the day field is shared.
Calling `set_day(1)` is therefore different from flooring to one common date.

## Month Length Is Checked Per Element

The argument must first lie in `[1, 31]`. It must then form a valid date with
*every* defined element's retained year and month. This second check matters
for mixed-month timelines:

```cpp
try {
  const auto months = pluss::timeline::make({
      "2024-01-10T00:00:00Z",
      "2024-02-10T00:00:00Z",
  });
  (void)months.set_day(30);
} catch (const pluss::Error&) {
  std::cout << "day is not valid for every month\n";
}
```

### Output

```text
day is not valid for every month
```

January could accept day 30, but February 2024 cannot. The method returns no
partial result. It never rolls into March and never clamps to month end.

## Absent And Empty Timelines

`NaT` stays `NaT`; there is no calendar date to edit. An empty timeline returns
an empty timeline with the same shape and timezone for valid arguments. The
argument range is still checked on empty input, so `empty.set_day(0)` throws
rather than hiding a programming error.

## Timezone Consequences

Replacement occurs in the current local calendar. If the new local date and
retained clock land in a daylight-saving gap, the method throws. If they land
in an overlap, the first occurrence is selected. Convert to the intended
timezone before calling `set_day()` whenever calendar boundaries are local.

## See Also

- [`day`](day.md): inspect the local day-of-month.
- [`set_month`](set_month.md): replace the month while retaining the day.
- [`date`](date.md): clear the time-of-day fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="seconds_between.html"><span class="label">Previous</span><span class="title">seconds_between</span></a>
  <a class="next-link" href="set_hour.html"><span class="label">Next</span><span class="title">set_hour</span></a>
</div>
<!-- pluss-hand-authored -->
