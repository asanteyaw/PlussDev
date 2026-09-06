# Timeline::year

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::year() const
```

Aliases: `y()`, `yr()`, `years()`, and `year_()`.

## Read The Civil Year

`year()` replaces every timestamp with the four-digit year shown by the
timeline's current timezone. It is an elementwise projection: rank, shape, and
element order are unchanged, while the result dtype is `int32`.

The word *civil* matters near a New Year boundary. `year()` answers which year
the local date belongs to. It does not return the ISO week-numbering year; a
date such as 1 January 2021 can therefore have `year() == 2021` and
`week() == 53` at the same time.

Missing timestamps become `-1`. Treat that value as a missing-field sentinel,
not as a historical year. Use `isvalid()` when the distinction matters.

## Example: Years, Boundaries, And Missing Values

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.year());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[2020, 2021, 2024, -1]
```

The first two values deliberately straddle midnight. The extraction is
elementwise, so neither ordering nor adjacency changes the answer.

## Timezone Changes The Calendar View

The stored instant and its displayed calendar year are different concepts.
`tz_convert()` preserves the instant but changes the local calendar used by
all field extractors. Near midnight, converting timezones can therefore change
the year. Convert first, extract second when grouping by a local reporting
calendar.

## Gotchas And Failure Modes

- `year()` does not mutate the timeline and does not return timestamps.
- A result of `-1` means the source element was `NaT`; it is not a valid year.
- Do not combine `year()` and `week()` as an ISO year-week key at New Year.
- The aliases are exact synonyms; they do not use different calendars.

## See Also

- [`month`](month.md): extract the month within the civil year.
- [`week`](week.md): extract the ISO week number.
- [`is_leap_year`](is_leap_year.md): classify each local calendar year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="searchsorted_left.html"><span class="label">Previous</span><span class="title">searchsorted_left</span></a>
  <a class="next-link" href="month.html"><span class="label">Next</span><span class="title">month</span></a>
</div>
<!-- pluss-hand-authored -->
