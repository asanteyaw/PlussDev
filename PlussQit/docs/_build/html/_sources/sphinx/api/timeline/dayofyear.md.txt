# Timeline::dayofyear

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::dayofyear() const
```

Aliases: `doy()`, `day_of_year()`, and `dayofyear_()`.

## Read The Ordinal Day

`dayofyear()` counts local calendar days from the start of each civil year.
It is one-based: 1 January is `1`; 31 December is `365` in a common year and
`366` in a leap year. The result is a shape-preserving `int32` Series, with
`-1` for missing timestamps.

Ordinal day is useful for annual seasonality because it reduces month and day
to one coordinate. It is not a stable elapsed-day counter across years: every
1 January resets it to `1`, and leap years insert day `366`.

## Example: Leap-Year And New-Year Boundaries

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.dayofyear());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[366, 1, 60, -1]
```

The leap day is ordinal day `60`: January contributes 31 days, so 29 February
is `31 + 29`.

## Example: Local Midnight Controls The Ordinal

The instant `2024-03-01T00:30:00Z` has day-of-year `61` in UTC. After conversion
to New York it is still 29 February locally and has day-of-year `60`. This is
why timezone conversion must precede local-seasonality feature extraction.

## Gotchas And Failure Modes

- The count starts at `1`, not `0`.
- Comparing ordinal days across leap and common years requires an explicit
  policy for dates after February.
- This field is periodic metadata, not a duration between timestamps.

## See Also

- [`day`](day.md)
- [`year`](year.md)
- [`is_leap_year`](is_leap_year.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html"><span class="label">Previous</span><span class="title">weekday</span></a>
  <a class="next-link" href="quarter.html"><span class="label">Next</span><span class="title">quarter</span></a>
</div>
<!-- pluss-hand-authored -->
